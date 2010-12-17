#include "../../../stdafx.h"

#include "video_decoder.h"

#include "../../../../common/exception/exceptions.h"
		
#if defined(_MSC_VER)
#pragma warning (push)
#pragma warning (disable : 4244)
#endif
extern "C" 
{
	#define __STDC_CONSTANT_MACROS
	#define __STDC_LIMIT_MACROS
	#include <libavformat/avformat.h>
}
#if defined(_MSC_VER)
#pragma warning (pop)
#endif

namespace caspar { namespace core { namespace ffmpeg{

struct video_decoder::implementation : boost::noncopyable
{
	implementation(AVCodecContext* codec_context) : codec_context_(codec_context){}

	safe_ptr<AVFrame> execute(const aligned_buffer& video_packet)
	{		
		safe_ptr<AVFrame> decoded_frame(avcodec_alloc_frame(), av_free);

		int frame_finished = 0;
		const int result = avcodec_decode_video(codec_context_, decoded_frame.get(), &frame_finished, video_packet.data(), video_packet.size());
		
		if(result < 0)
			BOOST_THROW_EXCEPTION(invalid_operation() << msg_info("avcodec_decode_video failed"));

		return decoded_frame;		
	}
	
	AVCodecContext* codec_context_;
};

video_decoder::video_decoder(AVCodecContext* codec_context) : impl_(new implementation(codec_context)){}
safe_ptr<AVFrame> video_decoder::execute(const aligned_buffer& video_packet){return impl_->execute(video_packet);}
}}}