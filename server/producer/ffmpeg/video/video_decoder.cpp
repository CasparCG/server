#include "../../../stdafx.h"

#include "video_decoder.h"
		
namespace caspar{ namespace ffmpeg{

struct video_decoder::implementation : boost::noncopyable
{
	video_packet_ptr execute(const video_packet_ptr& video_packet)
	{				
		int frame_finished = 0;
		int result = avcodec_decode_video(video_packet->codec_context, video_packet->decoded_frame.get(), &frame_finished, video_packet->data, video_packet->size);	

		return result >= 0 ? video_packet : nullptr;		
	}
};

video_decoder::video_decoder() : impl_(new implementation()){}
video_packet_ptr video_decoder::execute(const video_packet_ptr& video_packet){return impl_->execute(video_packet);}
}}