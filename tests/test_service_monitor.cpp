#include "service_monitor.h"

#include <filesystem>
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
    namespace fs = std::filesystem;
    const fs::path root = fs::temp_directory_path() / "health-monitor-proc";
    fs::create_directories(root / "1");
    fs::create_directories(root / "42");
    {
        std::ofstream((root / "1" / "comm").string()) << "systemd\n";
        std::ofstream((root / "42" / "comm").string()) << "sshd\n";
    }

    expect(ServiceMonitor::process_is_running("sshd", root.string()), "sshd should be found");
    expect(!ServiceMonitor::process_is_running("nginx", root.string()), "nginx should be absent");

    fs::remove_all(root);
    if (g_failures == 0) {
        std::cout << "test_service_monitor passed\n";
        return 0;
    }
    return 1;
}
