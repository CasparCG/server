#pragma once

#include <memory>

namespace caspar{
	
class frame;
typedef std::shared_ptr<frame> frame_ptr;
typedef std::shared_ptr<const frame> frame_const_ptr;
typedef std::unique_ptr<frame> frame_uptr;
typedef std::unique_ptr<const frame> frame_const_uptr;

struct frame_format_desc;
	
struct sound_channel_info;
typedef std::shared_ptr<sound_channel_info> sound_channel_info_ptr;
typedef std::unique_ptr<sound_channel_info> sound_channel_info_uptr;

class audio_chunk;
typedef std::shared_ptr<audio_chunk> audio_chunk_ptr;
typedef std::unique_ptr<audio_chunk> audio_chunk_uptr;

struct frame_factory;
typedef std::shared_ptr<frame_factory> frame_factory_ptr;
}