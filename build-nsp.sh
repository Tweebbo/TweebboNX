#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")"
./build.sh
python3 tools/nsp/package.py "$@"
