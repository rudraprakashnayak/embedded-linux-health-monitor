#include "disk_monitor.h"

#if defined(_WIN32)
#include <cstdint>
#else
#include <sys/statvfs.h>
#endif

HealthSample DiskMonitor::sample(const std::string& mount_path,
                                 double warning_percent,
                                 double critical_percent) const {
    HealthSample result;
    result.metric = "disk";
    result.unit = "%";

#if defined(_WIN32)
    (void)mount_path;
    (void)warning_percent;
    (void)critical_percent;
    result.level = HealthLevel::Unknown;
    result.message = "Disk sampling is only available on Linux";
    return result;
#else
    struct statvfs vfs {};
    if (statvfs(mount_path.c_str(), &vfs) != 0 || vfs.f_blocks == 0) {
        result.level = HealthLevel::Unknown;
        result.message = "Unable to stat filesystem at " + mount_path;
        return result;
    }

    const double total = static_cast<double>(vfs.f_blocks);
    const double available = static_cast<double>(vfs.f_bavail);
    const double used_percent = (1.0 - (available / total)) * 100.0;
    result.value = used_percent;
    result.level = classify_percent(used_percent, warning_percent, critical_percent);
    result.message = "Disk usage on " + mount_path + " is " + std::to_string(used_percent) + "%";
    return result;
#endif
}
