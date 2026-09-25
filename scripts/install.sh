#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
PREFIX="${PREFIX:-/usr/local}"
CONFIG_DIR="${CONFIG_DIR:-/etc/device-health-monitor}"

if [[ "$(id -u)" -ne 0 ]]; then
  echo "Run install.sh as root (sudo ./scripts/install.sh)" >&2
  exit 1
fi

command -v cmake >/dev/null || { echo "cmake is required" >&2; exit 1; }
command -v g++ >/dev/null || command -v clang++ >/dev/null || {
  echo "A C++17 compiler is required" >&2
  exit 1
}

BUILD_DIR="${ROOT}/build"
cmake -S "${ROOT}" -B "${BUILD_DIR}" -DCMAKE_BUILD_TYPE=Release
cmake --build "${BUILD_DIR}" --target device-health-monitor -j"$(nproc 2>/dev/null || echo 2)"

install -d "${PREFIX}/bin"
install -m 0755 "${BUILD_DIR}/device-health-monitor" "${PREFIX}/bin/device-health-monitor"
install -d "${CONFIG_DIR}"
if [[ ! -f "${CONFIG_DIR}/health_monitor.json" ]]; then
  install -m 0644 "${ROOT}/config/health_monitor.json" "${CONFIG_DIR}/health_monitor.json"
fi
touch /var/log/device-health-monitor.log
chmod 0644 /var/log/device-health-monitor.log

if command -v systemctl >/dev/null && [[ -d /lib/systemd/system ]]; then
  install -m 0644 "${ROOT}/systemd/device-health-monitor.service" \
    /lib/systemd/system/device-health-monitor.service
  systemctl daemon-reload
  systemctl enable --now device-health-monitor.service
  echo "Installed and started device-health-monitor.service"
else
  echo "systemd not found; binary installed to ${PREFIX}/bin/device-health-monitor"
fi
