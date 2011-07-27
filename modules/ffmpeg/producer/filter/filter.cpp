#include "../../stdafx.h"

#include "filter.h"

#include "../../ffmpeg_error.h"

#include <common/exception/exceptions.h>
#include <core/producer/frame/basic_frame.h>
#include <core/producer/frame/frame_factory.h>
#include <core/mixer/write_frame.h>

#include <boost/circular_buffer.hpp>

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
	
	implementation(const std::wstring& filters) 
		: filters_(narrow(filters))
	{
		std::transform(filters_.begin(), filters_.end(), filters_.begin(), ::tolower);
	}

	void push(const safe_ptr<AVFrame>& frame)
	{		
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

			PixelFormat pix_fmts[] = { PIX_FMT_BGRA, PIX_FMT_NONE };

			// Output
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

				for(size_t n = 0; n < 4; ++n)
				{
					frame->data[n]		= picref->data[n];
					frame->linesize[n]	= picref->linesize[n];
				}
				
				frame->format			= picref->format;
				frame->width			= picref->video->w;
				frame->height			= picref->video->h;
				frame->interlaced_frame = picref->video->interlaced;
				frame->top_field_first	= picref->video->top_field_first;
				frame->key_frame		= picref->video->key_frame;

				result.push_back(frame);
            }
        }

		return result;
	}
};

filter::filter(const std::wstring& filters) : impl_(new implementation(filters)){}
void filter::push(const safe_ptr<AVFrame>& frame) {impl_->push(frame);}
std::vector<safe_ptr<AVFrame>> filter::poll() {return impl_->poll();}
}