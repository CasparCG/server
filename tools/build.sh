#!/bin/bash

# Forward Linux builds to the native dependency-aware build helper.
set -euo pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
platform="$(uname -s)"

case "$platform" in
  Linux)
    exec "$script_dir/linux/build-with-system-deps" "$@"
    ;;
  *)
    echo "Unsupported build platform: $platform" >&2
    exit 1
    ;;
esac