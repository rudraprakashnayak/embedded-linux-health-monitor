# Test report

Built-in tests are C++ programs under `tests/` wired into CMake/`ctest`. They do not require root, systemd, or a live network. Each test writes a fixture, runs the production class, and checks the health classification.

## How to run

```bash
cmake -S . -B build
cmake --build build
cd build && ctest --output-on-failure
```

Expected: four tests pass (`test_cpu`, `test_memory`, `test_config`, `test_service_monitor`).

## Coverage

| Test | What it proves |
| --- | --- |
| `test_cpu` | `/proc/stat` parser warms up on the first sample, then reports positive usage after a second tick |
| `test_memory` | `MemAvailable` vs `MemTotal` yields ~80% used and WARNING at the default 80/90 bands |
| `test_config` | nested JSON keys populate poll interval, log path, CPU/memory/disk/temp/network/service fields |
| `test_service_monitor` | synthetic `/proc` tree: `sshd` is found, `nginx` is not |

Disk, temperature, and network monitors talk to `statvfs`, sysfs, `ping`, and `ip`. Those paths are exercised on a real Linux target with:

```bash
./build/device-health-monitor config/health_monitor.json
# in another shell, on a lab device you own:
./scripts/simulate_failure.sh cpu 8
```

Adjust `network.interface` before using the network simulation; `simulate_failure.sh network` toggles the link and needs root.

## Field checks on a board

1. Install with `sudo ./scripts/install.sh`.
2. Confirm `systemctl is-active device-health-monitor`.
3. `journalctl -u device-health-monitor -n 50` should show six metrics per poll.
4. If `sshd` is the watched service, `sudo systemctl stop sshd` should produce CRITICAL then a restart attempt (unless you change recovery to `log`).
5. `sudo ./scripts/uninstall.sh` removes the unit and binary.

## Limits

- Temperature UNKNOWN is normal on VMs with no thermal zones.
- Ping to `8.8.8.8` fails on air-gapped devices; set `check_host` to a local gateway or leave the interface-only check by pointing at a host that answers ICMP on-LAN.
- Unit tests for disk/network/temperature are not in this tree because they need live kernel interfaces; treat the field checklist above as the integration test.
