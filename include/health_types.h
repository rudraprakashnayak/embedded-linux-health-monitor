#pragma once

#include <string>

enum class HealthLevel {
    Ok,
    Warning,
    Critical,
    Unknown
};

inline const char* health_level_to_string(HealthLevel level) {
    switch (level) {
        case HealthLevel::Ok:
            return "OK";
        case HealthLevel::Warning:
            return "WARNING";
        case HealthLevel::Critical:
            return "CRITICAL";
        case HealthLevel::Unknown:
        default:
            return "UNKNOWN";
    }
}

inline HealthLevel classify_percent(double value, double warning, double critical) {
    if (value < 0.0) {
        return HealthLevel::Unknown;
    }
    if (value >= critical) {
        return HealthLevel::Critical;
    }
    if (value >= warning) {
        return HealthLevel::Warning;
    }
    return HealthLevel::Ok;
}

struct HealthSample {
    std::string metric;
    HealthLevel level = HealthLevel::Unknown;
    double value = -1.0;
    std::string unit;
    std::string message;
};

struct RecoveryResult {
    bool attempted = false;
    bool succeeded = false;
    std::string description;
};
