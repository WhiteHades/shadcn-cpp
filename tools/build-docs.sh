#!/usr/bin/env bash
set -euo pipefail
cd "$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
command -v npm >/dev/null || { echo 'Install Node.js 22 or later and npm first.' >&2; exit 1; }
python3 tools/check.py
bash tools/generate-api.sh
cd website
npm ci --ignore-scripts
npm run build
