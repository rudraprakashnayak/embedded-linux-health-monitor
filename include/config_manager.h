#pragma once

#include <string>
#include <vector>

struct CpuConfig {
    double warning_percent = 80.0;
    double critical_percent = 95.0;
    std::string recovery = "log";
};

struct MemoryConfig {
    double warning_percent = 80.0;
    double critical_percent = 90.0;
    std::string recovery = "drop_caches";
};

struct DiskConfig {
    std::string path = "/";
    double warning_percent = 80.0;
    double critical_percent = 90.0;
    std::string recovery = "log";
};

struct TemperatureConfig {
    double warning_celsius = 70.0;
    double critical_celsius = 85.0;
    std::string recovery = "set_powersave";
};

struct NetworkConfig {
    std::string interface_name = "eth0";
    std::string check_host = "8.8.8.8";
    std::string recovery = "restart_interface";
};

struct ServiceConfig {
    std::string name = "sshd";
    std::string process = "sshd";
    std::string recovery = "restart_service";
};

struct MonitorConfig {
    int poll_interval_seconds = 5;
    int recovery_cooldown_seconds = 30;
    std::string log_file = "/var/log/device-health-monitor.log";
    CpuConfig cpu;
    MemoryConfig memory;
    DiskConfig disk;
    TemperatureConfig temperature;
    NetworkConfig network;
    ServiceConfig service;
};

class ConfigManager {
public:
    bool load(const std::string& path, std::string& error);
    const MonitorConfig& config() const { return config_; }

    static MonitorConfig defaults();

private:
    MonitorConfig config_;

    static bool read_file(const std::string& path, std::string& out, std::string& error);
    static bool parse(const std::string& json, MonitorConfig& out, std::string& error);
};
