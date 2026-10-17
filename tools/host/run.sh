#!/usr/bin/env bash
# Builds and runs the desktop harness of the Switch client under Xvfb, then saves a screenshot
# of the frame in build/host/shot.png. The harness compiles the shipped sources (the shared core,
# source/nx/gfx.c and source/nx/conn.c) with a keyboard/POSIX main (tools/host/main.c).
#   SERVER=ws://127.0.0.1:8096 SHOT_MS=20000 tools/host/run.sh
set -euo pipefail
cd "$(dirname "$0")/../.."
podman image exists tweebbo-switch-host || podman build -t tweebbo-switch-host tools/host
mkdir -p build/host/data
[ -n "${SERVER:-}" ] && printf '%s\n' "$SERVER" > build/host/data/server.txt
[ -n "${ROOM:-}" ] && printf '%s\n' "$ROOM" > build/host/data/room.txt
rm -f build/host/shot.png
# romfs:/ paths resolve to a directory literally called "romfs:" next to the binary.
ln -sfn ../../romfs "build/host/romfs:"
podman run --rm --network host -v "$PWD":/work:z -w /work -e SHOT=/work/build/host/shot.png \
  -e SHOT_MS="${SHOT_MS:-20000}" -e QUIT_MS="${QUIT_MS:-25000}" tweebbo-switch-host bash -c '
  set -e
  gcc -std=gnu11 -O2 -Wall -DTWEEBBO_HOST -DTWEEBBO_DEVICE=2 -DAPP_VERSION="\"$(cat VERSION)-host\"" -D_GNU_SOURCE \
    -Isource $(sdl2-config --cflags) -o build/host/tweebbo-host \
    source/*.c source/gen/*.c source/nx/gfx.c source/nx/conn.c tools/host/main.c \
    $(sdl2-config --libs) -lSDL2_image -lmbedtls -lmbedx509 -lmbedcrypto -lpthread -lm
  cd build/host
  Xvfb :98 -screen 0 1280x720x24 >/dev/null 2>&1 & export DISPLAY=:98; sleep 1
  ./tweebbo-host data'
ls -la build/host/shot.png
