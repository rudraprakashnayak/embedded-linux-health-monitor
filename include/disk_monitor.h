#pragma once

#include "health_types.h"

#include <string>

class DiskMonitor {
public:
    HealthSample sample(const std::string& mount_path,
                        double warning_percent,
                        double critical_percent) const;
};
