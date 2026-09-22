#!/usr/bin/env bash
set -euo pipefail
cd "$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
command -v doxygen >/dev/null || { echo 'Install Doxygen first.' >&2; exit 1; }
command -v npm >/dev/null || { echo 'Install Node.js 22 or later and npm first.' >&2; exit 1; }
python3 tools/check.py
doxygen Doxyfile
cd website
if [[ -f package-lock.json ]]; then
  npm ci
else
  printf 'First documentation build: resolving dependencies. Commit package-lock.json after review.\n'
  npm install
fi
npm run build
