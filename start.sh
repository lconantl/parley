#!/usr/bin/env bash
set -euo pipefail
parley_root="$(CDPATH='' cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
cd "$parley_root"
if [[ "${1:-}" != "--check" && ! -f .env && -z "${BOT_TOKEN:-}" ]]; then
    echo "Скопируйте ваш прежний .env в эту папку или заполните .env.mock и сохраните как .env."
    exit 1
fi
if [[ ! -x build/parley ]]; then
    bash scripts/build-ubuntu.sh
fi
exec ./build/parley "$@"
