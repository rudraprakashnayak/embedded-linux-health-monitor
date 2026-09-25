#include "network_monitor.h"

#include <cstdlib>
#include <fstream>
#include <string>

namespace {

int run_command(const std::string& command) {
    return std::system(command.c_str());
}

}  // namespace

NetworkMonitor::NetworkMonitor(std::string sys_class_net) : sys_class_net_(std::move(sys_class_net)) {}

std::string NetworkMonitor::read_operstate(const std::string& interface_name) const {
    std::ifstream in(sys_class_net_ + "/" + interface_name + "/operstate");
    std::string state;
    if (in) {
        in >> state;
    }
    return state;
}

bool NetworkMonitor::ping_host(const std::string& host) const {
    if (host.empty()) {
        return true;
    }
    const std::string cmd = "ping -c 1 -W 2 " + host + " >/dev/null 2>&1";
    return run_command(cmd) == 0;
}

HealthSample NetworkMonitor::sample(const std::string& interface_name, const std::string& check_host) const {
    HealthSample result;
    result.metric = "network";
    result.unit = "state";

    const std::string state = read_operstate(interface_name);
    if (state.empty()) {
        result.level = HealthLevel::Unknown;
        result.message = "Interface " + interface_name + " was not found";
        return result;
    }

    if (state != "up") {
        result.value = 0.0;
        result.level = HealthLevel::Critical;
        result.message = "Interface " + interface_name + " operstate is " + state;
        return result;
    }

    const bool reachable = ping_host(check_host);
    result.value = reachable ? 1.0 : 0.0;
    if (!reachable) {
        result.level = HealthLevel::Critical;
        result.message = "Interface " + interface_name + " is up but " + check_host + " is unreachable";
        return result;
    }

    result.level = HealthLevel::Ok;
    result.message = "Interface " + interface_name + " is up and " + check_host + " is reachable";
    return result;
}

RecoveryResult NetworkMonitor::restart_interface(const std::string& interface_name) const {
    RecoveryResult result;
    result.attempted = true;
    result.description = "ip link set " + interface_name + " down/up";

    const int down = run_command("ip link set " + interface_name + " down >/dev/null 2>&1");
    const int up = run_command("ip link set " + interface_name + " up >/dev/null 2>&1");
    result.succeeded = (down == 0 && up == 0);
    if (!result.succeeded) {
        result.description += " (failed; try running as root)";
    }
    return result;
}
