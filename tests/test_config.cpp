#include "config_manager.h"

#include <cstdio>
#include <fstream>
#include <iostream>
#include <string>

static int g_failures = 0;

static void expect(bool cond, const char* msg) {
    if (!cond) {
        std::cerr << "FAIL: " << msg << '\n';
        ++g_failures;
    }
}

int main() {
    const std::string path = "health_monitor_test.json";
    std::ofstream out(path);
    out << R"({
  "poll_interval_seconds": 7,
  "log_file": "/tmp/health.log",
  "cpu": { "warning_percent": 70, "critical_percent": 90, "recovery": "log" },
  "memory": { "warning_percent": 75, "critical_percent": 88, "recovery": "drop_caches" },
  "disk": { "path": "/var", "warning_percent": 60, "critical_percent": 80, "recovery": "log" },
  "temperature": { "warning_celsius": 60, "critical_celsius": 80, "recovery": "set_powersave" },
  "network": { "interface": "wlan0", "check_host": "1.1.1.1", "recovery": "restart_interface" },
  "service": { "name": "cron", "process": "cron", "recovery": "restart_service" }
})";
    out.close();

    ConfigManager manager;
    std::string error;
    expect(manager.load(path, error), "config should load");
    const auto& cfg = manager.config();
    expect(cfg.poll_interval_seconds == 7, "poll interval");
    expect(cfg.log_file == "/tmp/health.log", "log file");
    expect(cfg.cpu.warning_percent == 70.0, "cpu warning");
    expect(cfg.memory.recovery == "drop_caches", "memory recovery");
    expect(cfg.disk.path == "/var", "disk path");
    expect(cfg.network.interface_name == "wlan0", "network iface");
    expect(cfg.service.name == "cron", "service name");

    std::remove(path.c_str());
    if (g_failures == 0) {
        std::cout << "test_config passed\n";
        return 0;
    }
    return 1;
}
