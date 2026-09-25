#include "cpu_monitor.h"

#include <fstream>
#include <sstream>

CpuMonitor::CpuMonitor(std::string proc_stat_path) : proc_stat_path_(std::move(proc_stat_path)) {}

bool CpuMonitor::read_times(std::uint64_t& idle, std::uint64_t& total) const {
    std::ifstream in(proc_stat_path_);
    if (!in) {
        return false;
    }

    std::string line;
    if (!std::getline(in, line)) {
        return false;
    }

    std::istringstream ss(line);
    std::string cpu;
    ss >> cpu;
    if (cpu != "cpu") {
        return false;
    }

    std::uint64_t user = 0, nice = 0, system = 0, idle_t = 0, iowait = 0, irq = 0, softirq = 0,
                  steal = 0, guest = 0, guest_nice = 0;
    ss >> user >> nice >> system >> idle_t >> iowait >> irq >> softirq >> steal >> guest >> guest_nice;

    idle = idle_t + iowait;
    total = user + nice + system + idle_t + iowait + irq + softirq + steal;
    return total > 0;
}

HealthSample CpuMonitor::sample(double warning_percent, double critical_percent) {
    HealthSample result;
    result.metric = "cpu";
    result.unit = "%";

    std::uint64_t idle = 0;
    std::uint64_t total = 0;
    if (!read_times(idle, total)) {
        result.level = HealthLevel::Unknown;
        result.message = "Unable to read " + proc_stat_path_;
        return result;
    }

    if (!have_previous_) {
        prev_idle_ = idle;
        prev_total_ = total;
        have_previous_ = true;
        result.level = HealthLevel::Unknown;
        result.message = "Warming up CPU sample";
        return result;
    }

    const std::uint64_t idle_delta = idle - prev_idle_;
    const std::uint64_t total_delta = total - prev_total_;
    prev_idle_ = idle;
    prev_total_ = total;

    if (total_delta == 0) {
        result.level = HealthLevel::Unknown;
        result.message = "No CPU time elapsed";
        return result;
    }

    const double usage = (1.0 - (static_cast<double>(idle_delta) / static_cast<double>(total_delta))) * 100.0;
    result.value = usage;
    result.level = classify_percent(usage, warning_percent, critical_percent);
    result.message = "CPU usage " + std::to_string(usage) + "%";
    return result;
}
