#!/bin/sh

RET=5

while [ $RET -eq 5 ]
do
  bin/casparcg "$@"
  RET=$?
done

