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

### FFmpeg STREAM consumer buffering

`STREAM` defaults to queues of 6 incoming frames and 16 encoded packets.
`FILE` retains defaults of 64 frames and 128 packets, including when its
destination is a UDP URL; its behavior is unchanged. For `STREAM`, both limits
can be overridden independently:

```text
ADD 1-603 STREAM "udp://239.1.1.1:5000?pkt_size=1316" -frame-buffer 6 -packet-buffer 16 -format mpegts
```

These are CasparCG options, not FFmpeg codec or UDP socket options. For a
preconfigured `<ffmpeg>` consumer, use `<frame-buffer>` and `<packet-buffer>`
alongside `<path>`, `<args>` and `<realtime>` in `casparcg.config`, not inside
`<args>`. Allowed integer ranges are 1..64 frames and 1..1024 packets.
Invalid values fail consumer creation. XML defaults to `<realtime>false</realtime>`;
`true` selects the STREAM defaults and realtime codec behavior. Buffer overrides
require `STREAM` or XML `<realtime>true</realtime>`; using them with `FILE` or
non-realtime XML consumers fails creation.

Queues drain as soon as processing permits; these capacities are not a fixed
playback delay or UDP packet pacing mechanism. Six queued inputs represent
about 100 ms at 59.94 inputs/second (fields for interlaced channels). Total
latency also includes in-flight encoding, muxing, packet and receiver buffers.
Larger frame limits can retain substantial uncompressed video memory.

STREAM input admission never waits for space, so a slow monitoring consumer does not
block SDI playout. When the input queue is full, the arriving frame and its
audio are discarded; timestamps still advance to preserve the source timeline.
A warning is emitted on the first rejection and at most once every five seconds
while rejections continue. Realtime consumer INFO/OSC state exposes `buffer/frames/capacity`,
`buffer/packets/capacity`, `buffer/dropped-frames` and
`buffer/dropped-audio-samples` (sample frames per audio channel). Counters reset
when the consumer is replaced. Packet queue saturation backpressures the
encoding worker, not channel playout; sustained overload can still cause input
loss. This change improves transient headroom, not guaranteed lossless audio.

The standalone configuration and queue-contract regression test can be run with
the project's Boost headers and TBB installed:

```sh
c++ -std=c++20 -pthread -Isrc tools/tests/ffmpeg_consumer_buffers_test.cpp -ltbb -ldl -o /tmp/ffmpeg-consumer-buffers-test
/tmp/ffmpeg-consumer-buffers-test
```

This tests parsing, queue limits, transient admission, sustained overload,
fractional-rate sample accounting and packet backpressure, not the complete
FFmpeg encoding pipeline. Actual audio continuity, latency, consumer replacement
and shutdown still require integration/on-site validation.

Development
-----------

See [BUILDING](BUILDING.md) for instructions on how to build the CasparCG Server from source manually.

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
