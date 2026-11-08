CasparCG Server
===============

Thank you for your interest in CasparCG Server, a professional software used to
play out and record professional graphics, audio and video to multiple outputs.
CasparCG Server has been in 24/7 broadcast production since 2006.

The CasparCG Server works on Windows and Linux.

System Requirements
-------------------

- A graphics card (GPU) capable of OpenGL 4.5 is required.
- An Nvidia GPU is recommended, but other GPU's will likely work fine.
- Intel and AMD CPU's have been tested and are known to work
- PCIE bandwidth is important between your GPU and CPU, as well as Decklink and CPU. Avoid chipset lanes when possible.

### Windows

 - Windows 11 is recommended
 - Windows 10 is supported as best effort

### Linux

 - Ubuntu 24.04 is recommended
 - Other distributions and releases will work but you will need to compile it yourself and have not been tested

Getting Started
---------------

1. Download a release from (http://casparcg.com/downloads).
   Alternatively, newer testing versions can be downloaded from (http://builds.casparcg.com) or [built from source](BUILDING.md)

2. Install any optional non-GPL modules
    - Flash template support (Windows only):

    1. Uninstall any previous version of the Adobe Flash Player using this file:
        (http://download.macromedia.com/get/flashplayer/current/support/uninstall_flash_player.exe)

    2. Download and unpack
        (http://download.macromedia.com/pub/flashplayer/installers/archive/fp_11.8.800.94_archive.zip)

    3. Install Adobe Flash Player 11.8.800.94 from the unpacked archive:
        fp_11.8.800.94_archive\11_8_r800_94\flashplayer11_8r800_94_winax.exe

3. Configure the server by editing the self-documented "casparcg.config" file in
   a text editor.

4.
   1. Windows: start `casparcg_auto_restart.bat`, or `casparcg.exe` and `scanner.exe` separately.
   1. Linux: start the `run.sh` program or use tools/linux/start_docker.sh to run within docker (documentation is at the top of the file).

5. Connect to the Server from a client software, such as the "CasparCG Client"
   which is available as a separate download.

Documentation
-------------

The most up-to-date documentation is always available at https://casparcg.com/docs/wiki

Ask questions in the forum: https://casparcgforum.org/

Development
-----------

See [BUILDING](BUILDING.md) for instructions on how to build the CasparCG Server from source manually.

Native route audio mapping
--------------------------

Native `route://` producers pass audio through unchanged unless `AUDIO_MAP` is
specified at creation. Entries use `destination=source` with zero-based audio
channel indices. When a map is active, destination channels not listed are
silent. Source channel buffers are not modified.
The same creation option works with `LOAD` and `LOADBG`; it is native route
processing, not the FFmpeg producer's `AF` option.

```text
PLAY 5-10 route://1 AUDIO_MAP "0=0,1=1"
PLAY 5-20 route://2 AUDIO_MAP "2=0,3=1"
PLAY 5-30 route://3 AUDIO_MAP "4=0,5=1"
PLAY 5-40 route://4 AUDIO_MAP "6=0,7=1"
```

Change a playing route without replacing its producer using `CALL`. This
atomically takes effect on the next source frame pair, after already queued
route audio has drained; `DEFAULT` restores normal
passthrough. Calling `AUDIO_MAP` without a value returns the current setting.
The effective mapping is also reported as `route/audio-map` in producer state.
The current source channel count is reported as `route/source-audio-channels`.
Mapped PCM uses the source's current channel stride after a format change;
selected indices no longer present produce silence until they return or the
mapping is updated. New updates are validated against the current source count.

```text
CALL 5-20 AUDIO_MAP "2=4,3=5"
CALL 5-20 AUDIO_MAP
CALL 5-20 AUDIO_MAP DEFAULT
```

Audio changes are sample-immediate at that frame boundary, with no fade or
additional buffering. Switching between unrelated waveforms can click; use a
short gain ramp in the controlling workflow when click-free transitions are
required.
Explicitly mapped routes mute repeated late frames rather than replaying stale
audio, while preserving their video, including mapped frames already buffered
before a `DEFAULT` update. Newly produced passthrough frames retain legacy behavior.

The FFmpeg consumer uses the channel's audio layout as its input. The following
consumer filter selects the mapped first eight channels as an eight-channel
7.1 stream, while keeping the route producers and video output continuous:

```text
ADD 5 FILE multiview.mkv -codec:v ffv1 -codec:a pcm_s24le -filter:a pan=7.1|FL=c0|FR=c1|FC=c2|LFE=c3|BL=c4|BR=c5|SL=c6|SR=c7
```

This eight-channel output example requires an FFmpeg build with the Matroska,
FFV1, and PCM encoders. The `pan` filter is applied by CasparCG's FFmpeg
consumer before encoding; it reduces the consumer's channel-format input to
the eight routed channels.

On-site verification: play a 16-channel PCM test file with a distinct tone on
each source channel, create the four routes above, and record the FFmpeg
consumer output. Run:

```text
ffprobe -v error -select_streams a:0 -show_entries stream=channels,channel_layout -of default=nw=1 multiview.mkv
```

It should report eight channels. The filter uses 7.1 channel ordering, but
Matroska PCM may report the layout as `unknown`; use audio indices, not speaker
labels, to identify monitoring pairs. Measure/listen to each output channel to
confirm that routes 1-4 occupy only output pairs 1-2, 3-4,
5-6, and 7-8.
Change route 2 with the `CALL` example and verify that only output channels 3-4
change on the next routed frame; try an invalid map and verify both the AMCP
error and unchanged effective mapping, then restore with `DEFAULT`. Keep a
source SDI output and its OSC meters active during this test and confirm they
are unchanged. Finally, test source loss/recovery and run a sustained 59.94
source-to-destination test while checking for stale audio, drift, or cadence
errors.

Channel audio matrix
--------------------

`MIXER <channel> AUDIO_MATRIX` provides independent weighted mixes for each
configured output audio channel. It works with normal media, native routes,
and the interleaved PCM interfaces of the pro-audio producer/consumer in
upstream CasparCG/server#1809 and CasparCG/server#1806. Device callbacks and
their one-based device `CHANNEL_MAP` options are unchanged.
The pro-audio consumer opts out of GPU-to-host video readback; its device channel
map is applied to the channel's final matrix PCM, not to individual producers.

```text
MIXER 5 AUDIO_MATRIX 0 "5:10:0:POST:1,5:20:4:PRE:0.5"
MIXER 5 AUDIO_MATRIX 1 "5:10:1:POST:1,5:20:5:PRE:0.5"
MIXER 5 AUDIO_MATRIX 2 "1:10:4:PRE:1"
MIXER 5 AUDIO_MATRIX 3 "1:10:5:PRE:1"
MIXER 5 AUDIO_MATRIX 4 "2:*:0:POST:1"
MIXER 5 AUDIO_MATRIX 5 "2:*:1:POST:1"
MIXER 5 AUDIO_MATRIX 6 SILENCE
MIXER 5 AUDIO_MATRIX 7 SILENCE
```

Each input is `source-channel:source-layer:audio-index:PRE|POST:linear-gain`.
Server channel and layer numbers use normal AMCP addressing. Destination and
source audio indices are zero-based. Gains are finite values from -16 to 16;
negative values invert polarity. Inputs add together; one input may feed many
outputs. Up to 64 inputs per output and 64 distinct source buses per destination
channel are supported.

`PRE` bypasses the outer layer's `MIXER VOLUME`, including layer-fader mute.
`POST` respects it and its existing gain ramps. Both retain producer/transition
audio transforms, including paused-frame silence. `*` selects the entire source
stage: PRE sums before each layer fader, POST sums after those faders. Neither tap
includes the source channel's master volume or matrix output: these are
**pre-master, pre-matrix** taps, preventing matrix feedback even between channels.
The destination master volume applies after the selected mix.
Layer taps see the producer's actual PCM, including any route `AUDIO_MAP` already
applied; use the original source channel/layer to select channels discarded by a
route's explicit map.

Only explicitly configured destinations are overridden. Other destinations
keep their legacy audio sum exactly; an explicit empty bus (`SILENCE`) is silent.
Updates replace a complete destination bus atomically at a channel processing
boundary, keeping the same source FIFO when a bus's gains/selections change.
No producer, video stream, or consumer is recreated. Invalid updates leave the
previous configuration in place. Use:

```text
MIXER 5 AUDIO_MATRIX
MIXER 5 AUDIO_MATRIX 2 DEFAULT
MIXER 5 AUDIO_MATRIX DEFAULT
INFO 5
```

The query returns the accepted configuration. `INFO`/OSC exposes the processing
configuration at `mixer/audio-matrix`, and destination audio meters reflect the
actual matrix output. Per-output `DEFAULT` restores legacy summing on that output;
bare `DEFAULT` restores legacy summing on the entire channel.

Cross-channel taps transfer only PCM through preallocated single-writer,
single-reader FIFOs. They do not create route producers, copy pixel data, or
render source video for the tap. Source producers still produce their normal
frames for other users; a source channel with no consumers does not run its
image compositor just to supply audio. Taps mix on engine channel threads, not
PortAudio callback threads.

All sources and destinations must use the same sample rate; mismatches are
rejected rather than silently resampled. Different video rates/field cadences
are accommodated by sample-count FIFO consumption, including 1080i5994 to
1080p5994 and faster sources feeding slower destinations. FIFO capacity is the
next power of two above two times the larger source/destination field cadence,
plus one source field cadence minus one sample. This covers destination reads
and publication bursts with bounded PCM storage, not a configurable video buffer.
Independent clocks can
still drift: underflow inserts silence and overflow discards queued stale PCM
at the next read; diagnostics report `audio-matrix-underflow/overflow`.
No asynchronous clock-drift correction or click-free crosspoint ramp is added.

Absent layers and missing audio publish silence; stopping a source thread drains
only bounded queued PCM before silence. Changes to source PCM channel count or
sample rate mute the affected tap and report `audio-matrix-format`. Reissue its
matrix configuration to create a tap for the new format. Producer and bus mixing
retain the engine's signed 32-bit saturation; keep adequate headroom when summing
several loud inputs. Video format changes with unchanged PCM dimensions are safe.

Automated tests cover deterministic four-pair isolation, weighted summing,
PRE/POST faders, fan-out, legacy/silent outputs, source-buffer isolation, missing
audio/recovery, format changes, overflow recovery, FIFO concurrency and 6,000
59.94 ticks (4,804,800 PCM frames), including interlaced-to-progressive transport.
Threaded channel tests exercise live matrix and native route updates, invalid
updates, unchanged producer/consumer instances, and audio-only sources without
compositing their video. They are engine tests, not a
hardware synchronization guarantee. On site, repeat the route verification above
using matrix commands, both pro-audio devices, source channels without consumers,
and a prolonged independent-clock test. Verify audio channel layout with the
same eight-channel FFmpeg consumer filter, and check no program SDI audio or
source meters change while updating destination mixes.

### Pro-audio device output

Configure the engine mix with `MIXER AUDIO_MATRIX`, then select physical device
outputs separately. For example:

```text
MIXER 5 AUDIO_MATRIX 0 "1:10:4:PRE:1,2:20:0:POST:0.5"
MIXER 5 AUDIO_MATRIX 1 "1:10:5:PRE:1,2:20:1:POST:0.5"
ADD 5 PROAUDIO DEVICE_NAME "Monitoring interface" CHANNEL_MAP "1,2" BUFFER_SIZE 512 LATENCY_MS 10
```

Here engine audio indices 0 and 1 feed device outputs 1 and 2. `CHANNEL_MAP`
is still one-based physical device addressing; it does not select source layers
or perform summing. Updating the matrix does not reopen the device. Additional
engine channels retain legacy mixing unless explicitly overridden; list more
device outputs if those channels should also be played. PortAudio callbacks use
the same shared SPSC PCM FIFO as the matrix transport, without callback-side
locks, allocation or matrix processing.

### Pro-audio capture into matrix buses

Capture physical inputs into engine channels on a source layer, then tap that
layer without creating a video route:

```text
PLAY 1-10 PROAUDIO DEVICE_NAME "Capture interface" CHANNEL_MAP "5,6,1,2" BUFFER_SIZE 512 LATENCY_MS 10
MIXER 5 AUDIO_MATRIX 0 "1:10:0:PRE:1"
MIXER 5 AUDIO_MATRIX 1 "1:10:1:PRE:1"
MIXER 5 AUDIO_MATRIX 2 "1:10:2:POST:1"
MIXER 5 AUDIO_MATRIX 3 "1:10:3:POST:1"
ADD 5 PROAUDIO DEVICE_NAME "Monitoring interface" CHANNEL_MAP "1,2,3,4"
```

The producer's one-based device inputs 5,6,1,2 become zero-based engine indices
0,1,2,3; remaining engine channels are zero-filled. Matrix indices select those
engine channels, not physical device inputs. PRE bypasses the source layer
volume, while POST follows it. Live matrix changes do not reopen the capture or
playback streams and do not change either device map.

The capture producer retains its existing transparent 1x1 frame for engine
compatibility. Cross-channel taps transport PCM only, never copy its video, and
do not invoke the source image compositor when that source has no consumers.
Capture conversion and matrix mixing remain on engine threads; the input device
callback only maps samples into preallocated scratch storage and the shared
FIFO. Independent capture/output clocks still require on-site drift testing;
this feature does not add asynchronous sample-rate correction.
The first positive audio read on playback or resume discards queued capture
from loading/paused time. That read can be silent until fresh input arrives;
zero-sample background previews do not consume the reset. Omitted device maps
use only the smaller engine/device channel count, with unused engine capture
channels silent. Explicit capture maps may repeat a physical input; explicit
output maps must have unique destinations.

License
---------

CasparCG Server is distributed under the GNU General Public License GPLv3 or
higher, see [LICENSE](LICENSE) for details.

CasparCG Server uses the following third party libraries:
- FFmpeg (http://ffmpeg.org/) under the GPLv2 Licence.
  FFmpeg is a trademark of Fabrice Bellard, originator of the FFmpeg project.
- Threading Building Blocks (http://www.threadingbuildingblocks.org/) library under the GPLv2 Licence.
- SFML (http://www.sfml-dev.org/) under the zlib/libpng License.
- GLEW (http://glew.sourceforge.net) under the modified BSD License.
- boost (http://www.boost.org/) under the Boost Software License, version 1.0.
