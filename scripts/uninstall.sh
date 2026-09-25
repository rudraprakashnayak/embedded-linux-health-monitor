#!/usr/bin/env bash
set -euo pipefail

if [[ "$(id -u)" -ne 0 ]]; then
  echo "Run uninstall.sh as root (sudo ./scripts/uninstall.sh)" >&2
  exit 1
fi

if command -v systemctl >/dev/null; then
  systemctl disable --now device-health-monitor.service >/dev/null 2>&1 || true
  rm -f /lib/systemd/system/device-health-monitor.service
  systemctl daemon-reload >/dev/null 2>&1 || true
fi

rm -f /usr/local/bin/device-health-monitor
echo "Removed device-health-monitor binary and systemd unit."
echo "Config left in /etc/device-health-monitor (delete manually if desired)."
