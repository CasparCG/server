#include "../../../stdafx.h"

#include "video_decoder.h"

#include "../../../frame/algorithm.h"
		
namespace caspar{ namespace ffmpeg{

struct video_decoder::implementation : boost::noncopyable
{
	video_packet_ptr execute(const video_packet_ptr& video_packet)
	{				
		if(video_packet->codec->id == CODEC_ID_RAWVIDEO) // TODO: doesnt sws_scale do this?
			common::image::shuffle(video_packet->frame->data(), video_packet->data, video_packet->size, 3, 2, 1, 0);
		else
		{
			video_packet->decoded_frame.reset(avcodec_alloc_frame(), av_free);

			int frame_finished = 0;
			if((-avcodec_decode_video(video_packet->codec_context, video_packet->decoded_frame.get(), &frame_finished, video_packet->data, video_packet->size)) > 0) 						
				video_packet->decoded_frame.reset();			
		}

		return video_packet;		
	}
};

video_decoder::video_decoder() : impl_(new implementation()){}
video_packet_ptr video_decoder::execute(const video_packet_ptr& video_packet){return impl_->execute(video_packet);}
}}