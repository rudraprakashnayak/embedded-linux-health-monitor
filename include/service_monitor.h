#pragma once

#include "health_types.h"

#include <string>

class ServiceMonitor {
public:
    HealthSample sample(const std::string& service_name, const std::string& process_name) const;
    RecoveryResult restart_service(const std::string& service_name) const;

    static bool process_is_running(const std::string& process_name,
                                   const std::string& proc_root = "/proc");

private:
    bool systemd_is_active(const std::string& service_name) const;
};
