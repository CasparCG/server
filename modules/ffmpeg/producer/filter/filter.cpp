#include "../../stdafx.h"

#include "filter.h"

#include "scalable_yadif.h"

#include "../../ffmpeg_error.h"

#include <common/exception/exceptions.h>
#include <core/producer/frame/basic_frame.h>
#include <core/producer/frame/frame_factory.h>
#include <core/mixer/write_frame.h>

#include <boost/circular_buffer.hpp>

#include <tbb/task_group.h>

#include <cstdio>
#include <sstream>

extern "C" 
{
	#define __STDC_CONSTANT_MACROS
	#define __STDC_LIMIT_MACROS
	#include <libavutil/avutil.h>
	#include <libavutil/imgutils.h>
	#include <libavfilter/avfilter.h>
	#include <libavfilter/avcodec.h>
	#include <libavfilter/avfiltergraph.h>
	#include <libavfilter/vsink_buffer.h>
	#include <libavfilter/vsrc_buffer.h>
}

namespace caspar {
	
struct filter::implementation
{
	std::string						filters_;
	std::shared_ptr<AVFilterGraph>	graph_;	
	AVFilterContext*				buffersink_ctx_;
	AVFilterContext*				buffersrc_ctx_;
	int								scalable_yadif_tag_;
		
	implementation(const std::wstring& filters) 
		: filters_(filters.empty() ? "null" : narrow(filters))
		, scalable_yadif_tag_(-1)
	{
		std::transform(filters_.begin(), filters_.end(), filters_.begin(), ::tolower);
	}

	~implementation()
	{
		release_scalable_yadif(scalable_yadif_tag_);
	}

	std::vector<safe_ptr<AVFrame>> execute(const std::shared_ptr<AVFrame>& frame)
	{
		push(frame);
		return poll();
	}

	void push(const std::shared_ptr<AVFrame>& frame)
	{		
		if(!frame)
			return;

		int errn = 0;	

		if(!graph_)
		{
			graph_.reset(avfilter_graph_alloc(), [](AVFilterGraph* p){avfilter_graph_free(&p);});
								
			// Input
			std::stringstream args;
			args << frame->width << ":" << frame->height << ":" << frame->format << ":" << 0 << ":" << 0 << ":" << 0 << ":" << 0; // don't care about pts and aspect_ratio
			errn = avfilter_graph_create_filter(&buffersrc_ctx_, avfilter_get_by_name("buffer"), "src", args.str().c_str(), NULL, graph_.get());
			if(errn < 0)
			{
				BOOST_THROW_EXCEPTION(caspar_exception() <<	msg_info(av_error_str(errn)) <<
					boost::errinfo_api_function("avfilter_graph_create_filter") <<	boost::errinfo_errno(AVUNERROR(errn)));
			}

			PixelFormat pix_fmts[] = 
			{
				PIX_FMT_YUV420P,
				PIX_FMT_YUVA420P,
				PIX_FMT_YUV422P,
				PIX_FMT_YUV444P,
				PIX_FMT_YUV411P,
				PIX_FMT_ARGB, 
				PIX_FMT_RGBA,
				PIX_FMT_ABGR,
				PIX_FMT_GRAY8,
				PIX_FMT_NONE
			};	
			// OPIX_FMT_BGRAutput
			errn = avfilter_graph_create_filter(&buffersink_ctx_, avfilter_get_by_name("buffersink"), "out", NULL, pix_fmts, graph_.get());
			if(errn < 0)
			{
				BOOST_THROW_EXCEPTION(caspar_exception() <<	msg_info(av_error_str(errn)) <<
					boost::errinfo_api_function("avfilter_graph_create_filter") << boost::errinfo_errno(AVUNERROR(errn)));
			}
			
			AVFilterInOut* outputs = avfilter_inout_alloc();
			AVFilterInOut* inputs  = avfilter_inout_alloc();

			outputs->name			= av_strdup("in");
			outputs->filter_ctx		= buffersrc_ctx_;
			outputs->pad_idx		= 0;
			outputs->next			= NULL;

			inputs->name			= av_strdup("out");
			inputs->filter_ctx		= buffersink_ctx_;
			inputs->pad_idx			= 0;
			inputs->next			= NULL;
			
			errn = avfilter_graph_parse(graph_.get(), filters_.c_str(), &inputs, &outputs, NULL);

			avfilter_inout_free(&inputs);
			avfilter_inout_free(&outputs);

			if(errn < 0)
			{
				BOOST_THROW_EXCEPTION(caspar_exception() <<	msg_info(av_error_str(errn)) <<
					boost::errinfo_api_function("avfilter_graph_parse") << boost::errinfo_errno(AVUNERROR(errn)));
			}
			
			errn = avfilter_graph_config(graph_.get(), NULL);
			if(errn < 0)
			{
				BOOST_THROW_EXCEPTION(caspar_exception() <<	msg_info(av_error_str(errn)) 
					<<	boost::errinfo_api_function("avfilter_graph_config") <<	boost::errinfo_errno(AVUNERROR(errn)));
			}

			for(size_t n = 0; n < graph_->filter_count; ++n)
			{
				auto filter_name = graph_->filters[n]->name;
				if(strstr(filter_name, "yadif") != 0)
					scalable_yadif_tag_ = make_scalable_yadif(graph_->filters[n]);
			}
		}
	
		errn = av_vsrc_buffer_add_frame(buffersrc_ctx_, frame.get(), 0);
		if(errn < 0)
		{
			BOOST_THROW_EXCEPTION(caspar_exception() << msg_info(av_error_str(errn)) <<
				boost::errinfo_api_function("av_vsrc_buffer_add_frame") << boost::errinfo_errno(AVUNERROR(errn)));
		}
	}

	std::vector<safe_ptr<AVFrame>> poll()
	{
		std::vector<safe_ptr<AVFrame>> result;

		if(!graph_)
			return result;
		
		while (avfilter_poll_frame(buffersink_ctx_->inputs[0])) 
		{
			AVFilterBufferRef *picref;
            av_vsink_buffer_get_video_buffer_ref(buffersink_ctx_, &picref, 0);
            if (picref) 
			{		
				safe_ptr<AVFrame> frame(avcodec_alloc_frame(), [=](AVFrame* p)
				{
					av_free(p);
					avfilter_unref_buffer(picref);
				});

				avcodec_get_frame_defaults(frame.get());	

				memcpy(frame->data,     picref->data,     sizeof(frame->data));
				memcpy(frame->linesize, picref->linesize, sizeof(frame->linesize));
				frame->format				= picref->format;
				frame->width				= picref->video->w;
				frame->height				= picref->video->h;
				frame->pkt_pos				= picref->pos;
				frame->interlaced_frame		= picref->video->interlaced;
				frame->top_field_first		= picref->video->top_field_first;
				frame->key_frame			= picref->video->key_frame;
				frame->pict_type			= picref->video->pict_type;
				frame->sample_aspect_ratio	= picref->video->sample_aspect_ratio;

				result.push_back(frame);
            }
        }

		return result;
	}
};

filter::filter(const std::wstring& filters) : impl_(new implementation(filters)){}
std::vector<safe_ptr<AVFrame>> filter::execute(const std::shared_ptr<AVFrame>& frame) {return impl_->execute(frame);}
}