#!/bin/bash

set -euo pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
platform="$(uname -s)"

case "$platform" in
  Linux)
    exec "$script_dir/linux/build-with-system-deps" "$@"
    ;;
  MINGW*|MSYS*|CYGWIN*)
    cd "$script_dir/.."
    MSYS_NO_PATHCONV=1 exec cmd.exe /d /c 'tools\build.bat' "$@"
    ;;
  *)
    echo "Unsupported build platform: $platform" >&2
    exit 1
    ;;
esac