#include "temperature_monitor.h"

#include <dirent.h>
#include <fstream>
#include <sstream>

TemperatureMonitor::TemperatureMonitor(std::string thermal_root)
    : thermal_root_(std::move(thermal_root)) {}

std::vector<double> TemperatureMonitor::read_zone_temps() const {
    std::vector<double> temps;
    DIR* dir = opendir(thermal_root_.c_str());
    if (!dir) {
        return temps;
    }

    while (dirent* entry = readdir(dir)) {
        const std::string name = entry->d_name;
        if (name.rfind("thermal_zone", 0) != 0) {
            continue;
        }
        const std::string path = thermal_root_ + "/" + name + "/temp";
        std::ifstream in(path);
        int milli_c = 0;
        if (in >> milli_c) {
            temps.push_back(static_cast<double>(milli_c) / 1000.0);
        }
    }
    closedir(dir);
    return temps;
}

HealthSample TemperatureMonitor::sample(double warning_celsius, double critical_celsius) const {
    HealthSample result;
    result.metric = "temperature";
    result.unit = "C";

    const auto temps = read_zone_temps();
    if (temps.empty()) {
        result.level = HealthLevel::Unknown;
        result.message = "No thermal zones found under " + thermal_root_;
        return result;
    }

    double max_temp = temps.front();
    for (double t : temps) {
        if (t > max_temp) {
            max_temp = t;
        }
    }

    result.value = max_temp;
    if (max_temp >= critical_celsius) {
        result.level = HealthLevel::Critical;
    } else if (max_temp >= warning_celsius) {
        result.level = HealthLevel::Warning;
    } else {
        result.level = HealthLevel::Ok;
    }
    std::ostringstream msg;
    msg << "Max SoC temperature " << max_temp << " C across " << temps.size() << " zone(s)";
    result.message = msg.str();
    return result;
}
