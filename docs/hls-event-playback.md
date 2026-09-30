# HLS EVENT playback

CasparCG can optionally tell FFmpeg which segment to use when it first opens an
active HLS playlist. This is useful when an `EVENT` playlist retains its full
history and playback should begin at, or seek relative to, the event's
beginning instead of FFmpeg's normal live position.

This behavior is opt-in. Omitting `HLS_START_INDEX` preserves the existing HLS
and live-stream behavior.

## AMCP syntax

Add `HLS_START_INDEX <signed-segment-index>` to an FFmpeg producer command:

```text
PLAY 1-10 "https://example.com/event.m3u8" HLS_START_INDEX 0
```

`HLS_START_INDEX 0` passes `live_start_index=0` to FFmpeg's HLS demuxer. It
selects the first segment currently listed in the playlist when the input is
opened.

The option is applied again whenever CasparCG resets or reopens the input.

### Seek from the beginning of an event

`SEEK` continues to use CasparCG frames. On a 50 fps channel, five minutes is
15,000 frames:

```text
PLAY 1-10 "https://example.com/event.m3u8" HLS_START_INDEX 0 SEEK 15000
```

This opens the input from the first available event segment and then seeks
approximately 300 seconds forward. HLS seeks normally resolve to a nearby
segment or keyframe, so exact frame accuracy is not expected.

Existing runtime seek commands retain the same event-relative timeline:

```text
CALL 1-10 SEEK 15000
```

### Negative segment indexes

The value is signed and is passed to FFmpeg unchanged. Negative indexes count
backward from the playlist's live edge. For example:

```text
PLAY 1-10 "https://example.com/event.m3u8" HLS_START_INDEX -3
```

starts at the third segment from the end, subject to FFmpeg's HLS behavior.

## Requirements and limitations

- The HLS origin must continue listing and serving every segment that should
  remain seekable. With a sliding-window playlist, index `0` means the first
  segment currently available, not the original event start.
- The URL does not need to end in `.m3u8`; signed, redirected, query-string,
  and extensionless HLS URLs are supported.
- Supplying the option for a non-HLS input does not force HLS detection. FFmpeg
  may reject or report the unused option.
- `HLS_START_INDEX` does not enable FFmpeg's `prefer_x_start` option and does
  not alter `#EXT-X-START` handling.
- CasparCG does not rewrite the playlist or recover segments that the origin
  no longer exposes.

## Diagnostics

At debug log level, every input open using this option records the value:

```text
av_input[https://example.com/event.m3u8] HLS live_start_index=0
```

If `HLS_START_INDEX` is omitted, CasparCG does not add `live_start_index` to
FFmpeg's input options.
