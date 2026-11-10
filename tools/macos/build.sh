#!/bin/sh
# Build CasparCG Server on macOS (Apple Silicon) using Homebrew dependencies.
# The mixer runs on the Vulkan backend through MoltenVK (Metal).
set -e
cd "$(dirname "$0")/../.."

brew install cmake ninja pkg-config boost ffmpeg tbb sfml glew openal-soft \
    molten-vk vulkan-headers vulkan-loader shaderc simde

cmake -S src -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
ninja -C build

echo "Built: build/shell/casparcg"
echo "Run it from a folder containing casparcg.config (set <accelerator>vulkan</accelerator> or auto)."
