#include "service_monitor.h"

#include <dirent.h>
#include <fstream>
#include <sstream>
#include <cstdlib>

bool ServiceMonitor::process_is_running(const std::string& process_name, const std::string& proc_root) {
    DIR* dir = opendir(proc_root.c_str());
    if (!dir) {
        return false;
    }

    bool found = false;
    while (dirent* entry = readdir(dir)) {
        const std::string name = entry->d_name;
        bool numeric = !name.empty();
        for (char c : name) {
            if (c < '0' || c > '9') {
                numeric = false;
                break;
            }
        }
        if (!numeric) {
            continue;
        }

        std::ifstream comm(proc_root + "/" + name + "/comm");
        std::string comm_name;
        if (comm && std::getline(comm, comm_name)) {
            if (comm_name == process_name) {
                found = true;
                break;
            }
        }
    }
    closedir(dir);
    return found;
}

bool ServiceMonitor::systemd_is_active(const std::string& service_name) const {
    const std::string cmd = "systemctl is-active --quiet " + service_name + ".service 2>/dev/null";
    return std::system(cmd.c_str()) == 0;
}

HealthSample ServiceMonitor::sample(const std::string& service_name, const std::string& process_name) const {
    HealthSample result;
    result.metric = "service";
    result.unit = "running";

    const bool systemd_ok = systemd_is_active(service_name);
    const bool proc_ok = process_is_running(process_name);

    if (systemd_ok || proc_ok) {
        result.value = 1.0;
        result.level = HealthLevel::Ok;
        result.message = "Service " + service_name + " / process " + process_name + " is running";
        return result;
    }

    result.value = 0.0;
    result.level = HealthLevel::Critical;
    result.message = "Service " + service_name + " is down and process " + process_name + " was not found";
    return result;
}

RecoveryResult ServiceMonitor::restart_service(const std::string& service_name) const {
    RecoveryResult result;
    result.attempted = true;
    result.description = "systemctl restart " + service_name;
    const int rc = std::system(("systemctl restart " + service_name + " >/dev/null 2>&1").c_str());
    result.succeeded = (rc == 0);
    if (!result.succeeded) {
        result.description += " (failed; systemd may be unavailable or permission denied)";
    }
    return result;
}
