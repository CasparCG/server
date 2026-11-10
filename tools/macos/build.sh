#!/bin/sh
# Build CasparCG Server on macOS (Apple Silicon) using Homebrew dependencies.
# The mixer runs on the Vulkan backend through MoltenVK (Metal).
set -e
cd "$(dirname "$0")/../.."

brew install cmake ninja pkg-config boost ffmpeg tbb sfml glew openal-soft \
    molten-vk vulkan-headers vulkan-loader shaderc simde

# Bundle the NDI runtime if it is installed (NDI Tools / NDI SDK), unless NDI_LIB is set explicitly.
if [ -z "$NDI_LIB" ]; then
    for CANDIDATE in /usr/local/lib/libndi.dylib "/Library/NDI SDK for Apple/lib/macOS/libndi.dylib"; do
        if [ -f "$CANDIDATE" ]; then NDI_LIB="$CANDIDATE"; break; fi
    done
fi

cmake -S src -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DCASPARCG_NDI_LIB="$NDI_LIB"
ninja -C build

echo "Built: build/CasparCG.app"
echo "Run it from a folder containing casparcg.config, e.g.:"
echo "  cd <your caspar folder> && $(pwd)/build/CasparCG.app/Contents/MacOS/casparcg"
echo "(build/shell/casparcg also works, but without HTML templates.)"
