#include "cpu_monitor.h"

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

static void write_file(const std::string& path, const std::string& contents) {
    std::ofstream out(path);
    out << contents;
}

int main() {
    const std::string path = "cpu_stat_fixture";
    write_file(path, "cpu  100 0 100 800 0 0 0 0 0 0\ncpu0 100 0 100 800 0 0 0 0 0 0\n");

    CpuMonitor monitor(path);
    const auto warmup = monitor.sample(80.0, 95.0);
    expect(warmup.level == HealthLevel::Unknown, "first sample should warm up");

    write_file(path, "cpu  200 0 200 850 0 0 0 0 0 0\ncpu0 200 0 200 850 0 0 0 0 0 0\n");
    const auto sample = monitor.sample(80.0, 95.0);
    expect(sample.level != HealthLevel::Unknown, "second sample should compute usage");
    expect(sample.value > 0.0, "usage should be positive");
    expect(sample.metric == "cpu", "metric name");

    std::remove(path.c_str());
    if (g_failures == 0) {
        std::cout << "test_cpu passed\n";
        return 0;
    }
    return 1;
}
