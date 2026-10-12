#!/usr/bin/env bash
# Baut und startet alle Host-Unit-Tests (PlatformIO "native" + Unity).
# Aufruf: test/run.sh   (aus beliebigem Verzeichnis)
# Optional: PIO=/pfad/zu/pio test/run.sh
set -euo pipefail
cd "$(dirname "$0")/native"
PIO="${PIO:-pio}"
command -v "$PIO" >/dev/null 2>&1 || { echo "pio nicht gefunden (pip install platformio oder PIO=... setzen)" >&2; exit 127; }
exec "$PIO" test -e native -v "$@"
