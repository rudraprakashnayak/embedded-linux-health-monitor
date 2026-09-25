#include "memory_monitor.h"

#include <cstdio>
#include <fstream>
#include <iostream>
#include <string>

static int g_failures = 0;

static void expect(bool cond, const char* msg) {
    if (!cond) {
        std::cerr << "FAIL: " << msg << '\n';
        ++g_failures;
    }
}

int main() {
    const std::string path = "meminfo_fixture";
    std::ofstream out(path);
    out << "MemTotal:       1000000 kB\n"
        << "MemFree:         100000 kB\n"
        << "MemAvailable:    200000 kB\n"
        << "Buffers:          10000 kB\n"
        << "Cached:           50000 kB\n";
    out.close();

    MemoryMonitor monitor(path);
    const auto sample = monitor.sample(80.0, 90.0);
    expect(sample.metric == "memory", "metric name");
    expect(sample.value > 79.0 && sample.value < 81.0, "usage should be ~80%");
    expect(sample.level == HealthLevel::Warning, "80% is warning");

    std::remove(path.c_str());
    if (g_failures == 0) {
        std::cout << "test_memory passed\n";
        return 0;
    }
    return 1;
}
