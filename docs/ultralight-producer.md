# Ultralight HTML Producer

## Overview

The Ultralight producer is a synchronous HTML renderer for CasparCG, designed
for faster-than-real-time offline rendering. It replaces CEF (Chromium Embedded
Framework) for templates that need deterministic, frame-accurate rendering.

The existing CEF-based HTML producer is preserved and remains the default for
real-time playout.

## Why not CEF?

CEF renders asynchronously on its own wall-clock timer (~25fps). This works for
real-time playout but creates two problems for offline rendering:

1. **Speed ceiling**: CEF can't render faster than wall-clock. The offline
   consumer achieves 3× real-time for video-only content but drops to ~1×
   when CEF templates are active.

2. **Frame drift**: CEF's `requestAnimationFrame` fires at wall-clock rate,
   not at the channel tick rate. In offline mode the channel ticks faster,
   so the overlay's frame counter drifts relative to the video.

`SendExternalBeginFrame` (CEF's external compositor control) is unreliable:
no 1:1 guarantee with `OnPaint`, known bugs across CEF versions, and no test
coverage upstream. See CEF Issue #2800 and CasparCG Issue #1177.

## Why Ultralight

[Ultralight](https://ultralig.ht/) is a lightweight WebKit fork with a
synchronous rendering API:

- `Renderer::Update()` — advances JS timers and `requestAnimationFrame`
- `Renderer::Render()` — composites one frame, blocks until done
- `BitmapSurface::LockPixels()` — direct BGRA pixel access

No internal clock, no async compositor. One `receive_impl()` call = one
fresh frame. The channel ticks as fast as the CPU can render.

## Template compatibility

Ultralight uses WebKit's layout engine and JavaScriptCore. Existing CasparCG
HTML templates work unchanged — same CG protocol:

| CG Command | JS Function |
|------------|-------------|
| CG PLAY    | `play()`    |
| CG STOP    | `stop()`    |
| CG NEXT    | `next()`    |
| CG UPDATE  | `update(data)` |
| CG REMOVE  | `remove()`  |
| CG INVOKE  | custom JS   |

Templates can read `window.__caspar_frame` for the current channel frame
number (injected per tick by both CEF and Ultralight producers).

## Known limitations vs CEF

- **No audio capture**: Ultralight has no equivalent to CEF's
  `OnAudioStreamPacket`. Templates with `<audio>`/`<video>` elements
  won't produce audio. Use dedicated audio producers instead.

- **No remote debugging**: No Chrome DevTools port. Preview and debug
  templates in a standard browser during development.

- **No WebGL in CPU mode**: GPU backend required for WebGL. Headless
  Docker environments use CPU-only rendering.

## License

Ultralight is proprietary (free for non-commercial, paid for commercial).
It is NOT bundled with CasparCG. Users download the SDK separately from
https://ultralig.ht/ and enable it at build time.

`ENABLE_ULTRALIGHT=OFF` by default. The GPL CasparCG distribution ships
without Ultralight, identical to how the Decklink SDK is handled.

## Building

```bash
# Download Ultralight SDK
curl -fSL -o ultralight-sdk.7z \
  "https://ultralight-sdk.sfo2.cdn.digitaloceanspaces.com/ultralight-sdk-latest-linux-x64.7z"
7z x ultralight-sdk.7z -o/opt/ultralight-sdk

# Build CasparCG with Ultralight
cmake -GNinja /source \
  -DENABLE_ULTRALIGHT=ON \
  -DULTRALIGHT_SDK_PATH=/opt/ultralight-sdk
```

## AMCP Usage

When Ultralight is enabled, it registers as the CG producer for `.html`
templates. Use CG commands as normal:

```
CG 1-10 ADD 0 my_template 1
CG 1-10 PLAY 0
CG 1-10 UPDATE 0 "{\"name\": \"value\"}"
CG 1-10 REMOVE 0
```

Or load explicitly via the producer:

```
PLAY 1-10 [ULTRALIGHT] my_template
```

## Source files

- `src/modules/ultralight/ultralight.cpp` — module init/uninit
- `src/modules/ultralight/producer/ultralight_producer.cpp` — synchronous frame_producer
- `src/modules/ultralight/producer/ultralight_cg_proxy.cpp` — CG command mapping
