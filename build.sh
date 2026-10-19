#!/usr/bin/env bash
# Builds Tweebbo for Nintendo Switch inside the devkitpro/devkita64 container.
#   ./build.sh            builds out/tweebbo.nro for the public hotel (wss://tweebbo.lol)
#   ./build.sh showcase   builds it for a hotel of this PC's Wi-Fi only (ws://<LAN IP>:8096)
#   ./build.sh clean
# When no hotel answers, the game offers to play offline. The toolchain and portlibs come from the image.
set -euo pipefail
cd "$(dirname "$0")"
IMAGE=docker.io/devkitpro/devkita64:latest

if [ "${1:-}" = clean ]; then
  podman run --rm -v "$PWD":/work:z -w /work "$IMAGE" bash -c 'make clean'
  rm -rf out
  exit 0
fi

[ -f romfs/gfx/atlas0.png ] || { echo "Assets are missing: run tools/fetch-assets.sh first"; exit 1; }

if [ "${1:-}" = showcase ]; then
  # Showcase: a hotel of the local network only (ws://<this PC's LAN IP>:8096).
  LAN_ONLY=1
  if [ -z "${TWEEBBO_SERVER:-}" ]; then
    LAN_IP=$(ip -4 route get 1.1.1.1 2>/dev/null | sed -n 's/.* src \([0-9.]*\).*/\1/p')
    : "${LAN_IP:?Could not find the LAN address of this PC: set TWEEBBO_SERVER=ws://IP:8096}"
    TWEEBBO_SERVER="ws://$LAN_IP:${SHOWCASE_PORT:-8096}"
  fi
  python3 - "$TWEEBBO_SERVER" <<'CHECK'
import ipaddress, sys, urllib.parse
u = urllib.parse.urlparse(sys.argv[1])
try:
    host = ipaddress.ip_address(u.hostname)
    ok = u.scheme == 'ws' and host.is_private and not u.path.strip('/')
except (TypeError, ValueError):
    ok = False
if not ok: raise SystemExit(f'TWEEBBO_SERVER must be ws://<private LAN IP>:port, got {sys.argv[1]}')
CHECK
else
  # Public release: the hotel at tweebbo.lol over TLS.
  LAN_ONLY=0
  TWEEBBO_SERVER=wss://tweebbo.lol
fi
echo "Hotel: $TWEEBBO_SERVER"
# The address is baked into the objects: rebuild them when it changes.
mkdir -p build out
echo "$TWEEBBO_SERVER $LAN_ONLY" | cmp -s - build/server.stamp || { rm -f build/*.o; echo "$TWEEBBO_SERVER $LAN_ONLY" > build/server.stamp; }
podman run --rm -e TWEEBBO_SERVER="$TWEEBBO_SERVER" -e TWEEBBO_LAN_ONLY="$LAN_ONLY" -v "$PWD":/work:z -w /work "$IMAGE" bash -lc 'make -j"$(nproc)" && cp tweebbo.nro tweebbo.nacp out/'
ls -la out/
