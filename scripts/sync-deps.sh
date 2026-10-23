#!/usr/bin/env bash
# Fetch Magnum and Corrade into third_party/ at the commits pinned in deps.lock.
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
while read -r name url commit; do
  [ -n "$name" ] || continue
  dir="$ROOT/third_party/$name"
  if [ ! -d "$dir/.git" ]; then
    mkdir -p "$dir"; git -C "$dir" init -q; git -C "$dir" remote add origin "$url"
  fi
  if [ "$(git -C "$dir" rev-parse HEAD 2>/dev/null || true)" != "$commit" ]; then
    git -C "$dir" fetch -q --depth 1 origin "$commit"
    git -C "$dir" checkout -q --detach FETCH_HEAD
  fi
  echo "$name at $(git -C "$dir" rev-parse HEAD)"
done < "$ROOT/deps.lock"
