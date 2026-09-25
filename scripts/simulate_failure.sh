#!/usr/bin/env bash
# Local test helper: briefly stress one monitored resource so the agent can react.
# Intended for a lab board or VM you own. Do not run on production devices.
set -euo pipefail

MODE="${1:-cpu}"
SECONDS_TO_RUN="${2:-8}"

echo "Simulating ${MODE} pressure for ${SECONDS_TO_RUN}s"

case "${MODE}" in
  cpu)
    yes >/dev/null &
    PID=$!
    sleep "${SECONDS_TO_RUN}"
    kill "${PID}" >/dev/null 2>&1 || true
    ;;
  memory)
    SECONDS_TO_RUN="${SECONDS_TO_RUN}" python3 - <<'PY' &
import time, os
blob = bytearray(64 * 1024 * 1024)
time.sleep(int(os.environ.get("SECONDS_TO_RUN", "8")))
PY
    sleep "${SECONDS_TO_RUN}"
    ;;
  disk)
    TMP="$(mktemp /tmp/health-monitor-sim.XXXXXX)"
    dd if=/dev/zero of="${TMP}" bs=1M count=32 status=none || true
    sleep "${SECONDS_TO_RUN}"
    rm -f "${TMP}"
    ;;
  network)
    IFACE="${IFACE:-eth0}"
    echo "Toggling ${IFACE} down/up (requires root)"
    ip link set "${IFACE}" down || true
    sleep 2
    ip link set "${IFACE}" up || true
    ;;
  service)
    SVC="${SVC:-sshd}"
    echo "Restarting ${SVC} (requires root)"
    systemctl restart "${SVC}" || true
    ;;
  *)
    echo "Usage: $0 [cpu|memory|disk|network|service] [seconds]" >&2
    exit 1
    ;;
esac

echo "Simulation finished"
