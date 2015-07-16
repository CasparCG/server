#!/bin/sh


# set threads for compilling
echo "How many cores do you want to use for compilling : "
read BUILD_PARALLEL_THREADS_NUMBER
echo "number of cores to compile is: $BUILD_PARALLEL_THREADS_NUMBER"

export BUILD_ARCHIVE_NAME="CasparCG Server"
export BUILD_PARALLEL_THREADS=$BUILD_PARALLEL_THREADS_NUMBER

./build-linux.sh

