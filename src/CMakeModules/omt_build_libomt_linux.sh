#!/bin/sh
# Builds libomt.so from https://github.com/openmediatransport/libomt on Linux.
# Run from the root of a checkout of that repo.
#
# Picks the build script matching the host architecture, then finds the built
# libomt.so under bin/ since dotnet publish's output path isn't fixed.
set -e

arch="$(uname -m)"
case "$arch" in
    x86_64|amd64)
        build_script=buildlinuxx64.sh
        ;;
    aarch64|arm64)
        build_script=buildlinuxarm64.sh
        ;;
    *)
        echo "omt_build_libomt_linux.sh: unsupported architecture '$arch'" >&2
        exit 1
        ;;
esac

cd build
bash "$build_script"
cd ..

found="$(find bin -name 'libomt.so' 2>/dev/null | head -n 1)"
if [ -z "$found" ]; then
    echo "omt_build_libomt_linux.sh: libomt.so not found under bin/ after 'dotnet publish'." >&2
    exit 1
fi

cp "$found" ./libomt.so
