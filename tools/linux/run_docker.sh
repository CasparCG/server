#!/bin/bash

if [ -z "$DISPLAY" ]; then 
  echo "WARNING: DISPLAY is not set"
fi

if [ ! -d /tmp/.X11-unix ]; then
  echo "WARNING: X11 socket not found"
fi

if [ ! -f /root/.Xauthority ]; then
  echo "WARNING: Xauthority not found"
fi

./run.sh
