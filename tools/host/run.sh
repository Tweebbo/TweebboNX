#!/usr/bin/env bash
# Builds and runs the desktop harness of the Switch client under Xvfb, then saves a screenshot
# of the frame in build/host/shot.png. The harness compiles the shipped sources (the shared core,
# source/nx/gfx.c and source/nx/conn.c) with a keyboard/POSIX main (tools/host/main.c).
#   SERVER=ws://127.0.0.1:8096 SHOT_MS=20000 tools/host/run.sh
set -euo pipefail
cd "$(dirname "$0")/../.."
podman image exists tweebbo-switch-host || podman build -t tweebbo-switch-host tools/host
host_profile=${HOST_PROFILE:-data}
[[ "$host_profile" =~ ^[a-zA-Z0-9_-]+$ ]] || { echo 'Use a simple isolated HOST_PROFILE name'; exit 2; }
mkdir -p "build/host/$host_profile"
host_run_root=/work/build/host
if [ "$host_profile" != data ]; then
  mkdir -p "build/host/run-$host_profile"
  ln -sfn ../../../romfs "build/host/run-$host_profile/romfs:"
  ln -sfn ../../../romfs/lang "build/host/run-$host_profile/app0:lang"
  host_run_root="/work/build/host/run-$host_profile"
fi
[ -n "${STORED_SERVER:-${SERVER:-}}" ] && printf '%s\n' "${STORED_SERVER:-$SERVER}" > "build/host/$host_profile/server.txt"
[ -n "${ROOM:-}" ] && printf '%s\n' "$ROOM" > "build/host/$host_profile/room.txt"
rm -f build/host/shot.png
# romfs:/ paths resolve to a directory literally called "romfs:" next to the binary.
ln -sfn ../../romfs "build/host/romfs:"
# The Vita UI profile resolves dictionaries through its app0: path.
ln -sfn ../../romfs/lang "build/host/app0:lang"
podman run --rm --network host -v "$PWD":/work:z -w /work -e SHOT=/work/build/host/shot.png \
  -e HOST_PROFILE="$host_profile" -e HOST_RUN_ROOT="$host_run_root" -e TWEEBBO_UPDATE_RESOURCES="${TWEEBBO_UPDATE_RESOURCES:-}" \
  -e FASTFOOD_GAME="${FASTFOOD_GAME:-}" \
  -e SNOW_GAME="${SNOW_GAME:-}" \
  -e SOAK_EXERCISE="${SOAK_EXERCISE:-}" -e PAD_EXERCISE="${PAD_EXERCISE:-}" -e UI_EXERCISE="${UI_EXERCISE:-}" -e HOST_DEVICE="${HOST_DEVICE:-2}" -e HOST_WIDTH="${HOST_WIDTH:-640}" -e HOST_HEIGHT="${HOST_HEIGHT:-360}" \
  -e EXERCISE="${EXERCISE:-}" -e HOST_CFLAGS="${HOST_CFLAGS:-}" -e HOST_VERSION="${HOST_VERSION:-}" -e TWEEBBO_UPDATE_MANIFEST="${TWEEBBO_UPDATE_MANIFEST:-}" -e TWEEBBO_UPDATE_TARGET="${TWEEBBO_UPDATE_TARGET:-}" -e ASAN_OPTIONS="${ASAN_OPTIONS:-}" -e SERVER="${SERVER:-}" -e OFFLINE_MS="${OFFLINE_MS:-}" -e SHOT_MS="${SHOT_MS:-20000}" -e QUIT_MS="${QUIT_MS:-25000}" \
  -e TAPS="${TAPS:-}" -e PADS="${PADS:-}" -e SAY="${SAY:-}" -e HOST_TV="${HOST_TV:-}" -e TAP_XY="${TAP_XY:-}" -e TAP_MS="${TAP_MS:-}" -e WALK_TO="${WALK_TO:-}" tweebbo-switch-host bash -c '
  set -e
  build_key=$( { printf "%s\n" "$HOST_CFLAGS" "$HOST_DEVICE" "$HOST_WIDTH" "$HOST_HEIGHT" "$SERVER" "$HOST_VERSION"; sha256sum VERSION source/*.c source/*.h source/*.inc source/gen/* vendor/quickjs/*.[ch] vendor/qrcodegen/*.[ch] source/nx/*.[ch] tools/host/*.[ch] tools/host/run.sh; } | sha256sum | cut -d" " -f1 )
  if [ ! -x build/host/tweebbo-host ] || [ "$(cat build/host/build-key 2>/dev/null)" != "$build_key" ]; then
  gcc -std=gnu11 -O2 ${HOST_CFLAGS:-} -Wall -DTWEEBBO_HOST -DTWEEBBO_DEVICE=$HOST_DEVICE -DSCREEN_W=$HOST_WIDTH -DSCREEN_H=$HOST_HEIGHT -DTWEEBBO_LAN_ONLY=1 -DDEFAULT_SERVER="\"${SERVER:-ws://127.0.0.1:8096}\"" -DAPP_VERSION="\"${HOST_VERSION:-$(cat VERSION)}-host\"" -D_GNU_SOURCE \
    -Isource -Ivendor/quickjs -fwrapv -fno-strict-aliasing -DCONFIG_VERSION='\''"2026-06-04"'\'' $(sdl2-config --cflags) -o build/host/tweebbo-host \
    source/*.c source/gen/*.c vendor/quickjs/*.c vendor/qrcodegen/*.c source/nx/gfx.c source/nx/audio-backend.c source/nx/conn.c tools/host/main.c \
    $(sdl2-config --libs) -lSDL2_image -lmbedtls -lmbedx509 -lmbedcrypto -ljansson -lz -lpthread -lm -Wl,--wrap=on_casino -Wl,--wrap=on_snow -Wl,--wrap=on_activity -Wl,--wrap=on_music -Wl,--wrap=audio_mix
  printf "%s\n" "$build_key" > build/host/build-key
  fi
  export SDL_AUDIODRIVER=dummy
  cd "$HOST_RUN_ROOT"
  Xvfb :98 -screen 0 1280x720x24 >/dev/null 2>&1 & export DISPLAY=:98; sleep 1
  /work/build/host/tweebbo-host "/work/build/host/$HOST_PROFILE"'
ls -la build/host/shot.png
