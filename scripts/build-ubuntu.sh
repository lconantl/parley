#!/usr/bin/env bash
set -euo pipefail
parley_root="$(CDPATH='' cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$parley_root"
parley_packages=(build-essential cmake libcurl4-openssl-dev libsqlite3-dev nlohmann-json3-dev ca-certificates)
parley_missing=()
for package in "${parley_packages[@]}"; do
    if ! dpkg-query -W -f='${Status}' "$package" 2>/dev/null | grep -q 'install ok installed'; then
        parley_missing+=("$package")
    fi
done
if (( ${#parley_missing[@]} )); then
    echo "Устанавливаются зависимости сборки Ubuntu: ${parley_missing[*]}"
    parley_sudo=()
    if [[ "$(id -u)" != "0" ]]; then
        parley_sudo=(sudo)
    fi
    "${parley_sudo[@]}" apt-get update
    "${parley_sudo[@]}" apt-get install -y "${parley_missing[@]}"
fi
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=OFF
cmake --build build --parallel 2
./build/parley --check
