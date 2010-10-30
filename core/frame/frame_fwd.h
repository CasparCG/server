#pragma once

#include <memory>

namespace caspar { namespace core {
	
class gpu_frame;
typedef std::shared_ptr<gpu_frame> gpu_frame_ptr;

struct frame_factory;
typedef std::shared_ptr<frame_factory> frame_factory_ptr;

struct frame_format_desc;
	
struct sound_channel_info;
typedef std::shared_ptr<sound_channel_info> sound_channel_info_ptr;
typedef std::unique_ptr<sound_channel_info> sound_channel_info_uptr;

class audio_chunk;
typedef std::shared_ptr<audio_chunk> audio_chunk_ptr;
typedef std::unique_ptr<audio_chunk> audio_chunk_uptr;

}}