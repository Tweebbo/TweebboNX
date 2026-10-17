#!/usr/bin/env bash
# Builds Tweebbo for Nintendo Switch inside the devkitpro/devkita64 container.
#   ./build.sh            builds out/tweebbo.nro
#   ./build.sh clean
# The Switch toolchain and the portlibs (SDL2, mbedTLS) come from the image;
# nothing is installed on the host.
set -euo pipefail
cd "$(dirname "$0")"
IMAGE=docker.io/devkitpro/devkita64:latest

if [ "${1:-}" = clean ]; then
  podman run --rm -v "$PWD":/work:z -w /work "$IMAGE" bash -c 'make clean'
  rm -rf out
  exit 0
fi

[ -f romfs/gfx/atlas0.png ] || { echo "Assets are missing: run tools/fetch-assets.sh first"; exit 1; }

mkdir -p out
podman run --rm -v "$PWD":/work:z -w /work "$IMAGE" bash -lc 'make -j"$(nproc)" && mkdir -p out && cp tweebbo.nro tweebbo.nacp out/'
ls -la out/
