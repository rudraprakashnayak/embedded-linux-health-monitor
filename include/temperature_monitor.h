#pragma once

#include "health_types.h"

#include <string>
#include <vector>

class TemperatureMonitor {
public:
    explicit TemperatureMonitor(std::string thermal_root = "/sys/class/thermal");

    HealthSample sample(double warning_celsius, double critical_celsius) const;

private:
    std::string thermal_root_;

    std::vector<double> read_zone_temps() const;
};
