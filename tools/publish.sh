#!/usr/bin/env bash
# Publish only this project. Requires an authenticated GitHub CLI on your machine.
set -euo pipefail
case "${1:-}" in
  --public|--private) visibility="$1" ;;
  *) printf 'Usage: bash tools/publish.sh --public|--private\n' >&2; exit 2 ;;
esac
for program in git gh python3; do
  command -v "$program" >/dev/null || { printf 'Install %s first.\n' "$program" >&2; exit 1; }
done
cd "$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
python3 tools/check.py
gh auth status >/dev/null
owner="$(gh api user --jq .login)"
repo="$owner/shadcn-cpp"
if [[ "$owner" != 'WhiteHades' ]]; then
  printf 'Expected WhiteHades, but GitHub CLI is signed in as %s. Switch accounts first.\n' "$owner" >&2
  exit 1
fi
if gh repo view "$repo" >/dev/null 2>&1; then
  printf '%s already exists. Nothing was changed. Review it before pushing manually.\n' "$repo" >&2
  exit 1
fi
if [[ ! -d .git ]]; then
  git init -b main
  git config user.name >/dev/null || { printf 'Set your Git author name before publishing.\n' >&2; exit 1; }
  git config user.email >/dev/null || { printf 'Set your Git author email before publishing.\n' >&2; exit 1; }
  git add -- .
  git commit -m 'Start shadcn-cpp 0.1.0 development'
fi
if [[ -n "$(git status --porcelain)" ]]; then
  printf 'Commit or stash local changes before publishing.\n' >&2; exit 1
fi
if git remote get-url origin >/dev/null 2>&1; then
  printf 'An origin remote already exists. Nothing was changed.\n' >&2; exit 1
fi
gh repo create "$repo" "$visibility" --source=. --remote=origin --push \
  --description 'A native C++ port of shadcn/ui, built one component at a time.'
gh repo edit "$repo" --add-topic cpp --add-topic qt6 --add-topic shadcn \
  --add-topic ui-components --add-topic native-gui
printf 'Published %s. Check its Actions tab before treating the native build as verified.\n' "$repo"
printf 'Version remains 0.1.0. No release tag was created.\n'
