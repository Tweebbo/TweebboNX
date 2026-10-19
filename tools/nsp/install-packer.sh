#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/../.."
revision=56698df456eac4d5076e85499fe045a90fba790b
target="$PWD/build/hacbrewpack-src"
if [ ! -d "$target/.git" ]; then
  git clone https://github.com/pplatoon/hacBrewPack.git "$target"
fi
git -C "$target" checkout --detach "$revision"
cp "$target/config.mk.template" "$target/config.mk"
make -C "$target" -j4
mkdir -p build/hacbrewpack
cp "$target/hacbrewpack" build/hacbrewpack/hacbrewpack
