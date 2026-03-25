# Offline Rendering

## Overview

The offline consumer renders CasparCG output to file as fast as the CPU allows,
with no real-time synchronisation. The channel pipeline self-throttles via
back-pressure from a bounded frame queue — no wall-clock, no sleep, no multiplier.

This enables deterministic rendering of compositions (video, graphics, audio)
at speeds significantly faster than real-time, limited only by CPU encoding
speed and source decode rate.

## Why

CasparCG's standard ffmpeg consumer is real-time — it drops frames if encoding
can't keep up and idles if encoding is faster. The DeckLink consumer is locked
to the SDI hardware clock. Neither allows faster-than-real-time rendering.

The offline consumer was designed for pre-rendering graphics output, batch
processing, and automated testing where real-time constraints don't apply.

## Architecture

```
Stage (producers)
    │  tick as fast as the queue has space
    ▼
Mixer (OpenGL GPU composite)
    │  frame readback via PBO, async
    ▼
offline_consumer::send()
    │  push to bounded_queue (blocks when full → back-pressure)
    │  resolve future immediately after push
    ▼
encoder_thread (dedicated)
    │  pop frame, encode video + audio
    ▼
packet_thread
    │  mux to container, write to disk
    ▼
av_write_trailer() on consumer removal
```

### Key design: `has_synchronization_clock() = true`

The output module (`output.cpp`) enforces a wall-clock sleep when no consumer
provides a sync clock. The offline consumer returns `true` for
`has_synchronization_clock()` to skip this sleep. The blocking queue push
provides the actual backpressure — the channel runs exactly as fast as the
encoder can drain frames.

### Bounded queue back-pressure

The frame queue depth (default 4, configurable) is the only tuning knob.
When the queue is full, `send()` blocks, which stalls the channel output
loop, which stalls the mixer and stage. The pipeline naturally self-throttles
to the slowest component (encoder, decoder, GPU, or I/O).

## AMCP Usage

```
# Add an offline consumer to channel 1:
ADD 1 OFFLINE /path/to/output.mp4 -codec:v libx264 -preset:v ultrafast -crf:v 18

# Remove the consumer (flushes encoder and writes mp4 trailer):
REMOVE 1 OFFLINE /path/to/output.mp4
```

### Config file (preconfigured consumer)

```xml
<channel>
    <video-mode>720p2500</video-mode>
    <consumers>
        <offline>
            <path>/output/render.mp4</path>
            <args>-codec:v prores_ks -profile:v 3 -codec:a pcm_s24le</args>
            <queue-depth>4</queue-depth>
        </offline>
    </consumers>
</channel>
```

## Frame-accurate start/stop

For recordings that start and stop at precise content boundaries, use
AMCP's `BEGIN`/`COMMIT` batching to atomically start playback and the
consumer on the same channel tick:

```
LOAD 1-1 my_video
CG 1-10 ADD 0 my_template 0

BEGIN
PLAY 1-1
CG 1-10 PLAY 0
ADD 1 OFFLINE /output/render.mp4 -codec:v libx264 -preset:v ultrafast
COMMIT
```

This ensures the first recorded frame is the first frame of playback.
Remove the consumer to stop recording and flush the file:

```
REMOVE 1 OFFLINE /output/render.mp4
```

## Performance

| Scenario (1080p25)          | Expected speed     |
|-----------------------------|--------------------|
| Video only, libx264 fast    | 3–8× real-time     |
| Video only, ProRes 422 HQ   | 5–15× real-time    |
| Video + CEF HTML templates  | ~1× real-time*     |
| Video + Ultralight templates| 2–3× real-time     |

\* CEF renders asynchronously at wall-clock speed, limiting offline throughput.
See [Ultralight HTML Producer](ultralight-producer.md) for the synchronous alternative.

## Source files

- `src/modules/ffmpeg/consumer/offline_consumer.cpp` — the consumer implementation
- `src/modules/ffmpeg/consumer/offline_consumer.h` — header
- `src/modules/ffmpeg/ffmpeg.cpp` — consumer registration
