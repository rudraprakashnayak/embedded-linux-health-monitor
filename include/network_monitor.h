#pragma once

#include "health_types.h"

#include <string>

class NetworkMonitor {
public:
    explicit NetworkMonitor(std::string sys_class_net = "/sys/class/net");

    HealthSample sample(const std::string& interface_name,
                        const std::string& check_host) const;

    RecoveryResult restart_interface(const std::string& interface_name) const;

private:
    std::string sys_class_net_;

    std::string read_operstate(const std::string& interface_name) const;
    bool ping_host(const std::string& host) const;
};
