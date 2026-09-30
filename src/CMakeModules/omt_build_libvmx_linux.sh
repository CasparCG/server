#!/bin/sh
# Builds libvmx.so from https://github.com/openmediatransport/libvmx on Linux.
# Run from the root of a checkout of that repo.
#
# Picks the build script matching the host architecture. These aren't just
# different flags: x86_64 uses vmxcodec_x86.cpp/vmxcodec_avx2.cpp, aarch64 uses
# a NEON-based vmxcodec_arm.cpp instead.
set -e

cd build

arch="$(uname -m)"
case "$arch" in
    x86_64|amd64)
        bash buildlinuxx64.sh
        ;;
    aarch64|arm64)
        bash buildlinuxarm64.sh
        ;;
    *)
        echo "omt_build_libvmx_linux.sh: unsupported architecture '$arch'" >&2
        exit 1
        ;;
esac
