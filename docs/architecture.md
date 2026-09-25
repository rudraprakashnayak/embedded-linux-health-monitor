# Architecture

## Role

`device-health-monitor` is a long-running Linux process. Each poll interval it samples six subsystems, maps raw values onto health levels, logs a line per metric, and optionally runs one recovery command when a metric is CRITICAL and not in cooldown.

```
                 health_monitor.json
                          |
                          v
                     ConfigManager
                          |
          +---------------+---------------+
          v               v               v
     CpuMonitor     MemoryMonitor     DiskMonitor
  TemperatureMonitor  NetworkMonitor  ServiceMonitor
          |               |               |
          +---------------+---------------+
                          v
                    main control loop
                     |            |
                   Logger     Recovery
                 (file+stderr)  (cooldown map)
```

## Sampling

- **CPU.** Two consecutive reads of the aggregate `cpu` line in `/proc/stat`. Usage is `1 - idle_delta/total_delta`. The first tick is UNKNOWN (warmup).
- **Memory.** `MemAvailable / MemTotal` (falls back to Free+Buffers+Cached). Reported as used percent.
- **Disk.** `statvfs` on the configured path; used percent from `f_blocks` vs `f_bavail`.
- **Temperature.** Maximum millidegree reading across `thermal_zone*` converted to Celsius.
- **Network.** Sysfs `operstate` must be `up`, then `ping -c 1` to `check_host`.
- **Service.** Healthy if `systemctl is-active` succeeds **or** a `/proc/<pid>/comm` match exists. That covers boards where the process is alive but the unit name differs.

## Recovery

| Metric | Action name | Effect |
| --- | --- | --- |
| cpu | `log` | no command |
| memory | `drop_caches` | `sync` then write `3` to `/proc/sys/vm/drop_caches` |
| disk | `log` | no command (never auto-deletes files) |
| temperature | `set_powersave` | write `powersave` to each CPU `scaling_governor` |
| network | `restart_interface` | `ip link set <iface> down` then `up` |
| service | `restart_service` | `systemctl restart <name>` |

Cooldown is per metric. A failed recovery is logged and still consumes the cooldown so a tight loop cannot hammer systemd or the NIC.

## Process model

- Single thread, poll + 1-second sleep slices so SIGTERM is handled quickly.
- systemd unit: `Type=simple`, `Restart=always`, `Nice=10`.
- Config path: CLI argument, else `HEALTH_MONITOR_CONFIG`, else `/etc/device-health-monitor/health_monitor.json`, else `config/health_monitor.json`.

## Code map

| File | Responsibility |
| --- | --- |
| `src/main.cpp` | daemon loop, signals, recovery dispatch |
| `src/config_manager.cpp` | small JSON subset parser (objects, strings, numbers) |
| `src/logger.cpp` | timestamped file + stderr log |
| `src/*_monitor.cpp` | one metric each, injectable proc/sys paths for tests |
