#!/bin/sh

RET=5

while [ $RET -eq 5 ]
do
  LD_LIBRARY_PATH=lib bin/casparcg "$@"
  RET=$?
done

