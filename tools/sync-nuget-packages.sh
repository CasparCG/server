#!/bin/bash

SRC_DIR=$PWD/src
BUILD_DIR=$1

if [ -z ${1+x} ]; then
    echo "Expected build directory"
    exit 1
fi

cd $BUILD_DIR

for d in */ ; do
    # echo "$d"
    if [ -f "$d/packages.config" ]; then
        echo "syncing $d"
        cp "${d}packages.config" "$SRC_DIR/${d}packages.config"
    fi
    if [ "$d" == "modules/" ]; then 
        for e in $d*/ ; do
            # echo "$e"
            if [ -f "${e}packages.config" ]; then
                echo "syncing $e"
                cp "${e}packages.config" "$SRC_DIR/${e}packages.config"
            fi
        done
    fi
done
