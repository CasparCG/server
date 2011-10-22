#pragma once

#include <common/memory/safe_ptr.h>

#include <core/video_format.h>
#include <core/producer/frame/pixel_format.h>
#include <core/mixer/audio/audio_mixer.h>


#if defined(_MSC_VER)
#pragma warning (push)
#pragma warning (disable : 4244)
#endif
extern "C" 
{
	#include <libavutil/pixfmt.h>
	#include <libavcodec/avcodec.h>
}
#if defined(_MSC_VER)
#pragma warning (pop)
#endif

#include <agents.h>
#include <semaphore.h>

struct AVFrame;
struct AVFormatContext;
struct AVPacket;

namespace caspar {

namespace core {

struct pixel_format_desc;
class write_frame;
struct frame_factory;

}

namespace ffmpeg {
	
// Dataflow

class token
{
	safe_ptr<Concurrency::semaphore> semaphore_;
public:
	token(const safe_ptr<Concurrency::semaphore>& semaphore)
		: semaphore_(semaphore)
	{
		semaphore_->acquire();
	}

	~token()
	{
		semaphore_->release();
	}
};

template <typename T>
struct message
{
	message(const T& payload = T(), const std::shared_ptr<token>& token = nullptr)
		: payload(payload)
		, token(token)
	{
	}

	T						payload;
	std::shared_ptr<token>	token;
};

template<typename T>
safe_ptr<message<T>> make_message(const T& payload, const std::shared_ptr<token>& token = nullptr)
{
	return make_safe<message<T>>(payload, token);
}

typedef safe_ptr<message<std::shared_ptr<AVPacket>>>			packet_message_t;
typedef safe_ptr<message<std::shared_ptr<AVFrame>>>				video_message_t;
typedef safe_ptr<message<std::shared_ptr<core::audio_buffer>>>	audio_message_t;
typedef safe_ptr<message<safe_ptr<core::basic_frame>>>			frame_message_t;
	
const std::shared_ptr<AVPacket>& loop_packet(int index);
const std::shared_ptr<AVPacket>& eof_packet(int index);

const std::shared_ptr<AVFrame>& loop_video();
const std::shared_ptr<AVFrame>& empty_video();
const std::shared_ptr<AVFrame>& eof_video();
const std::shared_ptr<core::audio_buffer>& loop_audio();
const std::shared_ptr<core::audio_buffer>& empty_audio();
const std::shared_ptr<core::audio_buffer>& eof_audio();

// Utils

static const PixelFormat	CASPAR_PIX_FMT_LUMA = PIX_FMT_MONOBLACK; // Just hijack some unual pixel format.

core::field_mode::type		get_mode(AVFrame& frame);
core::pixel_format::type	get_pixel_format(PixelFormat pix_fmt);
core::pixel_format_desc		get_pixel_format_desc(PixelFormat pix_fmt, size_t width, size_t height);
int							make_alpha_format(int format); // NOTE: Be careful about CASPAR_PIX_FMT_LUMA, change it to PIX_FMT_GRAY8 if you want to use the frame inside some ffmpeg function.
safe_ptr<core::write_frame> make_write_frame(const void* tag, const safe_ptr<AVFrame>& decoded_frame, const safe_ptr<core::frame_factory>& frame_factory, int hints);

void fix_meta_data(AVFormatContext& context);

std::shared_ptr<AVPacket> create_packet();

safe_ptr<AVCodecContext> open_codec(AVFormatContext& context,  enum AVMediaType type, int& index);
safe_ptr<AVFormatContext> open_input(const std::wstring& filename);

}}