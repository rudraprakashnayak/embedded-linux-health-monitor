#include "memory_monitor.h"

#include <cstdint>
#include <fstream>
#include <sstream>
#include <string>
#include <unordered_map>

MemoryMonitor::MemoryMonitor(std::string proc_meminfo_path)
    : proc_meminfo_path_(std::move(proc_meminfo_path)) {}

HealthSample MemoryMonitor::sample(double warning_percent, double critical_percent) const {
    HealthSample result;
    result.metric = "memory";
    result.unit = "%";

    std::ifstream in(proc_meminfo_path_);
    if (!in) {
        result.level = HealthLevel::Unknown;
        result.message = "Unable to read " + proc_meminfo_path_;
        return result;
    }

    std::unordered_map<std::string, std::uint64_t> values;
    std::string line;
    while (std::getline(in, line)) {
        std::istringstream ss(line);
        std::string key;
        std::uint64_t kb = 0;
        ss >> key >> kb;
        if (!key.empty() && key.back() == ':') {
            key.pop_back();
        }
        values[key] = kb;
    }

    const std::uint64_t total = values["MemTotal"];
    if (total == 0) {
        result.level = HealthLevel::Unknown;
        result.message = "MemTotal is missing or zero";
        return result;
    }

    std::uint64_t available = 0;
    if (values.count("MemAvailable")) {
        available = values["MemAvailable"];
    } else {
        available = values["MemFree"] + values["Buffers"] + values["Cached"];
    }

    const double used_percent = (1.0 - (static_cast<double>(available) / static_cast<double>(total))) * 100.0;
    result.value = used_percent;
    result.level = classify_percent(used_percent, warning_percent, critical_percent);
    result.message = "RAM usage " + std::to_string(used_percent) + "%";
    return result;
}
