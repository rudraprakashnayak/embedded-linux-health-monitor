# Embedded Linux Device Health Monitor & Auto-Recovery Agent

A background supervisor for Linux boards and appliances that continuously monitors six critical health signals, classifies system state as OK / WARNING / CRITICAL, and runs bounded recovery actions when metrics stay unhealthy.

## Project Description

This project is a lightweight, zero-dependency C++17 daemon designed for embedded Linux devices — routers, IoT gateways, industrial controllers, and edge appliances. It runs as a systemd service in the background, polling CPU usage, RAM consumption, disk space, SoC temperature, network connectivity, and critical service health at configurable intervals.

When a metric crosses its WARNING or CRITICAL threshold, the agent logs the event and optionally triggers an automated recovery action (e.g., dropping page cache for memory pressure, restarting a failed service, or toggling a network interface). All thresholds, cooldowns, and recovery policies are defined in a single JSON config file — no recompilation needed.

Built with only Linux/POSIX APIs and the C++ STL, it compiles to a small static binary suitable for resource-constrained embedded targets.

## Outcomes & Results

- **Proactive fault detection**: Six health signals monitored continuously — issues are caught and logged before they cause outages, with OK/WARNING/CRITICAL classification for clear severity visibility
- **Automated self-healing**: Recovery actions (cache drop, service restart, network interface reset, CPU governor change) run automatically on CRITICAL events, reducing mean-time-to-recovery without human intervention
- **Safe recovery boundaries**: Cooldown timers prevent recovery loops; disk recovery never deletes user files; memory and network actions require root — safety is built in, not bolted on
- **Zero external dependencies**: Compiles with only GCC/Clang + CMake + pthread, producing a small binary that runs on any Linux 4.x+ kernel — ideal for embedded targets with minimal package managers
- **Production deployment ready**: systemd service file, install/uninstall scripts, and a failure simulation script for testing — drop it on a board and it runs
- **Tested and documented**: 4 unit test suites covering CPU, memory, config, and service monitoring, plus a full architecture doc and test report

## What it monitors

| Signal | Source | Default recovery on CRITICAL |
| --- | --- | --- |
| CPU usage | `/proc/stat` | log only |
| RAM usage | `/proc/meminfo` | drop page cache |
| Disk usage | `statvfs` on configured mount | log only |
| SoC temperature | `/sys/class/thermal/thermal_zone*/temp` | set `powersave` CPU governor |
| Network status | `operstate` + ping | `ip link set` down/up |
| Critical service | `systemctl is-active` and `/proc/<pid>/comm` | `systemctl restart` |

Thresholds, interface name, ping target, and the watched service all live in JSON. No third-party libraries.

## Repository layout

```
embedded-linux-health-monitor/
├── README.md
├── CMakeLists.txt
├── .gitignore
├── config/
│   └── health_monitor.json
├── include/
│   ├── cpu_monitor.h
│   ├── memory_monitor.h
│   ├── disk_monitor.h
│   ├── temperature_monitor.h
│   ├── network_monitor.h
│   ├── service_monitor.h
│   ├── logger.h
│   └── config_manager.h
├── src/
│   ├── main.cpp
│   ├── cpu_monitor.cpp
│   ├── memory_monitor.cpp
│   ├── disk_monitor.cpp
│   ├── temperature_monitor.cpp
│   ├── network_monitor.cpp
│   ├── service_monitor.cpp
│   ├── logger.cpp
│   └── config_manager.cpp
├── systemd/
│   └── device-health-monitor.service
├── scripts/
│   ├── install.sh
│   ├── uninstall.sh
│   └── simulate_failure.sh
├── tests/
│   ├── test_cpu.cpp
│   ├── test_memory.cpp
│   ├── test_config.cpp
│   └── test_service_monitor.cpp
└── docs/
    ├── architecture.md
    └── test-report.md
```

## Build

Linux with CMake 3.10+, a C++17 compiler, and pthread:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)
```

Foreground run (uses `config/health_monitor.json` if `/etc/device-health-monitor/` is empty):

```bash
./build/device-health-monitor config/health_monitor.json
```

On many Ethernet boards the default interface is `eth0`. Change `network.interface` in JSON to `wlan0`, `enp0s3`, or whatever `ip link` shows.

## Install as a systemd service

```bash
sudo ./scripts/install.sh
sudo journalctl -u device-health-monitor.service -f
```

Uninstall:

```bash
sudo ./scripts/uninstall.sh
```

## Tests

```bash
cmake -S . -B build
cmake --build build
cd build && ctest --output-on-failure
```

`scripts/simulate_failure.sh` can generate short CPU, memory, disk, network, or service pressure on a machine you own so you can watch the agent log a WARNING/CRITICAL and, where configured, attempt recovery.

## Recovery safety

- Recovery for a metric is skipped while its cooldown (`recovery_cooldown_seconds`) is active.
- Disk recovery never deletes user files; it only logs.
- Memory cache drop and interface/service restart need root.
- Stop with SIGINT or SIGTERM; systemd uses the same signals.

See [docs/architecture.md](docs/architecture.md) for the control loop and [docs/test-report.md](docs/test-report.md) for how the unit tests map to each monitor.

## License

MIT
