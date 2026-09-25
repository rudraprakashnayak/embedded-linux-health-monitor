#pragma once

#include "health_types.h"

#include <cstdint>
#include <string>

class CpuMonitor {
public:
    explicit CpuMonitor(std::string proc_stat_path = "/proc/stat");

    HealthSample sample(double warning_percent, double critical_percent);

private:
    std::string proc_stat_path_;
    bool have_previous_ = false;
    std::uint64_t prev_idle_ = 0;
    std::uint64_t prev_total_ = 0;

    bool read_times(std::uint64_t& idle, std::uint64_t& total) const;
};
