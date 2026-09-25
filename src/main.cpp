#include "config_manager.h"
#include "cpu_monitor.h"
#include "disk_monitor.h"
#include "health_types.h"
#include "logger.h"
#include "memory_monitor.h"
#include "network_monitor.h"
#include "service_monitor.h"
#include "temperature_monitor.h"

#include <chrono>
#include <csignal>
#include <fstream>
#include <map>
#include <string>
#include <thread>
#include <atomic>
#include <cstdlib>

namespace {

std::atomic<bool> g_running{true};

void on_signal(int) {
    g_running = false;
}

void write_sysfs(const std::string& path, const std::string& value) {
    std::ofstream out(path);
    if (out) {
        out << value;
    }
}

RecoveryResult recover_memory() {
    RecoveryResult result;
    result.attempted = true;
    result.description = "drop page cache via /proc/sys/vm/drop_caches";
    std::system("sync >/dev/null 2>&1");
    std::ofstream out("/proc/sys/vm/drop_caches");
    if (!out) {
        result.succeeded = false;
        result.description += " (permission denied)";
        return result;
    }
    out << "3\n";
    result.succeeded = static_cast<bool>(out);
    return result;
}

RecoveryResult recover_temperature() {
    RecoveryResult result;
    result.attempted = true;
    result.description = "set CPU frequency governor to powersave";
    bool any = false;
    for (int i = 0; i < 16; ++i) {
        const std::string path =
            "/sys/devices/system/cpu/cpu" + std::to_string(i) + "/cpufreq/scaling_governor";
        std::ifstream probe(path);
        if (!probe) {
            continue;
        }
        any = true;
        write_sysfs(path, "powersave");
    }
    result.succeeded = any;
    if (!any) {
        result.description += " (no cpufreq governors found)";
    }
    return result;
}

void maybe_recover(const HealthSample& sample,
                   const std::string& action,
                   Logger& logger,
                   std::map<std::string, std::chrono::steady_clock::time_point>& last_recovery,
                   int cooldown_seconds,
                   NetworkMonitor& network,
                   ServiceMonitor& service,
                   const MonitorConfig& cfg) {
    if (sample.level != HealthLevel::Critical || action == "log" || action.empty()) {
        return;
    }

    const auto now = std::chrono::steady_clock::now();
    const auto it = last_recovery.find(sample.metric);
    if (it != last_recovery.end() &&
        now - it->second < std::chrono::seconds(cooldown_seconds)) {
        logger.warning("Skipping recovery for " + sample.metric + " (cooldown active)");
        return;
    }

    RecoveryResult result;
    if (sample.metric == "memory" && action == "drop_caches") {
        result = recover_memory();
    } else if (sample.metric == "temperature" && action == "set_powersave") {
        result = recover_temperature();
    } else if (sample.metric == "network" && action == "restart_interface") {
        result = network.restart_interface(cfg.network.interface_name);
    } else if (sample.metric == "service" && action == "restart_service") {
        result = service.restart_service(cfg.service.name);
    } else {
        logger.info("No automatic recovery handler for " + sample.metric + " action=" + action);
        return;
    }

    last_recovery[sample.metric] = now;
    if (result.succeeded) {
        logger.warning("Recovery succeeded: " + result.description);
    } else {
        logger.error("Recovery failed: " + result.description);
    }
}

void emit(const Logger& logger, const HealthSample& sample) {
    const std::string line = std::string(health_level_to_string(sample.level)) + " " + sample.metric +
                             ": " + sample.message;
    switch (sample.level) {
        case HealthLevel::Critical:
            logger.critical(line);
            break;
        case HealthLevel::Warning:
            logger.warning(line);
            break;
        case HealthLevel::Unknown:
            logger.warning(line);
            break;
        case HealthLevel::Ok:
        default:
            logger.info(line);
            break;
    }
}

std::string default_config_path() {
    const char* env = std::getenv("HEALTH_MONITOR_CONFIG");
    if (env && *env) {
        return env;
    }
    std::ifstream etc("/etc/device-health-monitor/health_monitor.json");
    if (etc) {
        return "/etc/device-health-monitor/health_monitor.json";
    }
    return "config/health_monitor.json";
}

}  // namespace

int main(int argc, char** argv) {
    std::signal(SIGINT, on_signal);
    std::signal(SIGTERM, on_signal);

    const std::string config_path = (argc > 1) ? argv[1] : default_config_path();

    ConfigManager configs;
    std::string error;
    if (!configs.load(config_path, error)) {
        Logger fallback;
        fallback.error("Failed to load config from " + config_path + ": " + error);
        return 1;
    }

    const MonitorConfig& cfg = configs.config();
    Logger logger(cfg.log_file, true);
    logger.info("Device health monitor starting with config " + config_path);
    logger.info("Poll interval " + std::to_string(cfg.poll_interval_seconds) + "s");

    CpuMonitor cpu;
    MemoryMonitor memory;
    DiskMonitor disk;
    TemperatureMonitor temperature;
    NetworkMonitor network;
    ServiceMonitor service;
    std::map<std::string, std::chrono::steady_clock::time_point> last_recovery;

    while (g_running) {
        const auto cpu_s = cpu.sample(cfg.cpu.warning_percent, cfg.cpu.critical_percent);
        emit(logger, cpu_s);
        maybe_recover(cpu_s, cfg.cpu.recovery, logger, last_recovery, cfg.recovery_cooldown_seconds,
                      network, service, cfg);

        const auto mem_s = memory.sample(cfg.memory.warning_percent, cfg.memory.critical_percent);
        emit(logger, mem_s);
        maybe_recover(mem_s, cfg.memory.recovery, logger, last_recovery, cfg.recovery_cooldown_seconds,
                      network, service, cfg);

        const auto disk_s = disk.sample(cfg.disk.path, cfg.disk.warning_percent, cfg.disk.critical_percent);
        emit(logger, disk_s);
        maybe_recover(disk_s, cfg.disk.recovery, logger, last_recovery, cfg.recovery_cooldown_seconds,
                      network, service, cfg);

        const auto temp_s =
            temperature.sample(cfg.temperature.warning_celsius, cfg.temperature.critical_celsius);
        emit(logger, temp_s);
        maybe_recover(temp_s, cfg.temperature.recovery, logger, last_recovery,
                      cfg.recovery_cooldown_seconds, network, service, cfg);

        const auto net_s = network.sample(cfg.network.interface_name, cfg.network.check_host);
        emit(logger, net_s);
        maybe_recover(net_s, cfg.network.recovery, logger, last_recovery, cfg.recovery_cooldown_seconds,
                      network, service, cfg);

        const auto svc_s = service.sample(cfg.service.name, cfg.service.process);
        emit(logger, svc_s);
        maybe_recover(svc_s, cfg.service.recovery, logger, last_recovery, cfg.recovery_cooldown_seconds,
                      network, service, cfg);

        for (int i = 0; i < cfg.poll_interval_seconds && g_running; ++i) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }

    logger.info("Device health monitor stopped");
    return 0;
}
