#pragma once

#include "health_types.h"

#include <string>

class MemoryMonitor {
public:
    explicit MemoryMonitor(std::string proc_meminfo_path = "/proc/meminfo");

    HealthSample sample(double warning_percent, double critical_percent) const;

private:
    std::string proc_meminfo_path_;
};
