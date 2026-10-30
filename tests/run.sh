#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
podman image exists tweebbo-switch-host || podman build -t tweebbo-switch-host tools/host
podman run --rm -v "$PWD":/work:z -v "$PWD/../Tweebbo-Vita":/Tweebbo-Vita:ro,z -w /work tweebbo-switch-host bash -c '
  set -e
  tmp=$(mktemp -d)
  trap '\''rm -rf "$tmp"'\'' EXIT
  flags="-std=gnu11 -g -O1 -fsanitize=address,undefined -fno-omit-frame-pointer -Isource"
  gcc $flags -ffunction-sections -fdata-sections tests/power.c source/world.c -Wl,--gc-sections -lm -o "$tmp/power"
  "$tmp/power"
  gcc $flags -ffunction-sections -fdata-sections tests/http-idle.c -Wl,--gc-sections -lpthread -lm -o "$tmp/http-idle"
  "$tmp/http-idle"
  gcc $flags tests/input-repeat.c -o "$tmp/input-repeat"
  "$tmp/input-repeat"
  gcc $flags -ffunction-sections -fdata-sections tests/bed-pose.c -Wl,--gc-sections -lm -o "$tmp/bed-pose"
  "$tmp/bed-pose"
  gcc $flags tests/storage.c source/nx/log.c source/nx/store.c -Wl,--wrap=rename -lpthread -o "$tmp/storage"
  gcc $flags -ffunction-sections -fdata-sections tests/cache.c $(sdl2-config --cflags --libs) -lSDL2_image -lm -Wl,--gc-sections -o "$tmp/cache"
  gcc $flags tests/connection.c -lpthread -o "$tmp/connection"
  gcc $flags -ffunction-sections -fdata-sections tests/identity-session.c -Wl,--gc-sections -lpthread -o "$tmp/identity-session"
  gcc $flags -ffunction-sections -fdata-sections -DTWEEBBO_DEVICE=2 tests/devices.c source/world.c source/proto.c -Wl,--gc-sections -lm -o "$tmp/devices"
  gcc $flags -ffunction-sections -fdata-sections -DTWEEBBO_DEVICE=3 tests/devices.c source/world.c source/proto.c -Wl,--gc-sections -lm -o "$tmp/devices-vita"
  for device in 2 3; do
    canvas="-DSCREEN_W=640 -DSCREEN_H=360"
    [ "$device" = 2 ] || canvas="-DSCREEN_W=480 -DSCREEN_H=272"
    gcc $flags $canvas -DTWEEBBO_DEVICE=$device -ffunction-sections -fdata-sections tests/room-input.c source/world.c -Wl,--gc-sections -ljansson -lm -o "$tmp/room-input-$device"
    gcc $flags $canvas -DTWEEBBO_DEVICE=$device -ffunction-sections -fdata-sections tests/ui.c tests/ui-services.c source/language.c source/audio.c source/client_music.c source/client_identity.c source/client_bar.c source/client_pool.c source/client_casino.c source/client_trade.c source/client_pets.c source/pets.c source/client_games.c source/client_social.c source/client_build.c source/catalog.c source/avatar.c source/paylink.c source/update.c source/incremental.c vendor/qrcodegen/qrcodegen.c source/gen/assets.c -Wl,--gc-sections -ljansson -lmbedtls -lmbedx509 -lmbedcrypto -lz -lm -o "$tmp/ui-$device"
  done
  gcc $flags tests/audio.c source/audio.c -lpthread -o "$tmp/audio"
  gcc $flags tests/text.c source/text.c source/gen/font.c -lm -o "$tmp/text"
  gcc $flags tests/vita-cache.c -o "$tmp/vita-cache"
  gcc $flags tests/social.c tests/english.c -ljansson -o "$tmp/social"
  gcc $flags tests/games.c tests/english.c -o "$tmp/games"
  gcc $flags -ffunction-sections -fdata-sections tests/casino.c tests/english.c -Wl,--gc-sections -ljansson -lm -o "$tmp/casino"
  gcc $flags -ffunction-sections -fdata-sections tests/trade.c tests/english.c -Wl,--gc-sections -ljansson -o "$tmp/trade"
  gcc $flags tests/finishes.c source/finishes.c -lm -o "$tmp/finishes"
  gcc $flags -Ivendor/quickjs -fwrapv -fno-strict-aliasing -DCONFIG_VERSION=\"test\" tests/snow.c tests/english.c source/audio.c vendor/quickjs/*.c -ljansson -lm -lpthread -o "$tmp/snow"
  ln -s /work/romfs "$tmp/romfs:"
  cd "$tmp"
  ./snow
  ./devices
  ./devices-vita
  ./room-input-2
  ./room-input-3
  ./ui-2
  ./ui-3
  ./text
  ./audio
  ./vita-cache
  ./social
  ./games
  ./casino
  ./trade
  ./finishes
  ./connection
  ./identity-session
  ./storage
  SDL_VIDEODRIVER=dummy ./cache
  cd /work
  python3 tests/incremental.py
  python3 tests/incremental-generator.py
  python3 tests/incremental-worker.py
'
