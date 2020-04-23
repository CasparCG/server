#!/bin/bash

CMD="docker run -it"

# bind AMCP Ports
CMD="$CMD -p 5250:5250"

# passthrough config
CMD="$CMD -v $PWD/casparcg.config:/opt/casparcg/casparcg.config:ro"

# passthrough media folder
CMD="$CMD -v $PWD/media:/opt/casparcg/media"

## DO NOT EDIT BELOW THIS LINE

HAS_NVIDIA_RUNTIME=$(docker info | grep -i nvidia)
if [ ! -z "$HAS_NVIDIA_RUNTIME" ]; then
  CMD="$CMD --runtime=nvidia"
else
  # assume intel, so setup for that
  CMD="$CMD --device /dev/dri"
fi

DECKLINK_API_SO="/usr/lib/libDeckLinkAPI.so"
if [ -f "$DECKLINK_API_SO" ]; then
  CMD="$CMD -v $DECKLINK_API_SO:$DECKLINK_API_SO:ro"

  for dev in /dev/blackmagic/*; do
    if [ -f "$dev" ]; then
      CMD="$CMD --device $dev"
    fi
  done
fi

if [ ! -z "$XAUTHORITY" ]; then
  CMD="$CMD -v $XAUTHORITY:/root/.Xauthority:ro"
elif [ -f "$HOME/.Xauthority" ]; then
  CMD="$CMD -v $HOME/.Xauthority:/root/.Xauthority:ro"
else
  echo "Failed to find Xauthority file"
  exit 9
fi

CMD="$CMD -e DISPLAY=$DISPLAY -v /tmp/.X11-unix:/tmp/.X11-unix:ro"

CMD="$CMD casparcg/server"

echo "Executing command: $CMD"

# Run it!
$CMD
