#include "../stdafx.h"

#include "deinterlacer.h"

#include "../ffmpeg_error.h"
#include "util.h"

#include <common/exception/exceptions.h>
#include <core/producer/frame/basic_frame.h>
#include <core/producer/frame/frame_factory.h>
#include <core/mixer/write_frame.h>

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
	#include <libavfilter/vsrc_buffer.h>
	#include <libavfilter/avfiltergraph.h>
}

namespace caspar {
	
struct deinterlacer::implementation
{
	std::shared_ptr<AVFilterGraph> graph_;
	safe_ptr<core::frame_factory> frame_factory_;
	AVFilterContext* video_in_filter_;
	AVFilterContext* video_out_filter_;
		
	implementation(const safe_ptr<core::frame_factory>& frame_factory)
		: frame_factory_(frame_factory)
	{			
	}

	std::vector<safe_ptr<core::write_frame>> execute(const safe_ptr<AVFrame>& frame)
	{
		std::vector<safe_ptr<core::write_frame>> result;
		
		int errn = 0;	

		if(!graph_)
		{
			graph_.reset(avfilter_graph_alloc(), [](AVFilterGraph* p){avfilter_graph_free(&p);});
			

			// Input
			std::stringstream buffer_ss;
			buffer_ss << frame->width << ":" << frame->height << ":" << frame->format << ":" << 0 << ":" << 0 << ":" << 0 << ":" << 0; // don't care about pts and aspect_ratio
			errn = avfilter_graph_create_filter(&video_in_filter_, avfilter_get_by_name("buffer"), "src", buffer_ss.str().c_str(), NULL, graph_.get());
			if(errn < 0)
			{
				BOOST_THROW_EXCEPTION(caspar_exception() <<	msg_info(av_error_str(errn)) <<
					boost::errinfo_api_function("avfilter_graph_create_filter") <<	boost::errinfo_errno(AVUNERROR(errn)));
			}

			// Output
			errn = avfilter_graph_create_filter(&video_out_filter_, avfilter_get_by_name("nullsink"), "out", NULL, NULL, graph_.get());
			if(errn < 0)
			{
				BOOST_THROW_EXCEPTION(caspar_exception() <<	msg_info(av_error_str(errn)) <<
					boost::errinfo_api_function("avfilter_graph_create_filter") << boost::errinfo_errno(AVUNERROR(errn)));
			}
			
			AVFilterInOut* outputs = reinterpret_cast<AVFilterInOut*>(av_malloc(sizeof(AVFilterInOut)));
			AVFilterInOut* inputs  = reinterpret_cast<AVFilterInOut*>(av_malloc(sizeof(AVFilterInOut)));

			outputs->name			= av_strdup("in");
			outputs->filter_ctx		= video_in_filter_;
			outputs->pad_idx		= 0;
			outputs->next			= NULL;

			inputs->name			= av_strdup("out");
			inputs->filter_ctx		= video_out_filter_;
			inputs->pad_idx			= 0;
			inputs->next			= NULL;
			
			std::stringstream yadif_ss;
			yadif_ss << "yadif=" << "1:" << frame->top_field_first;

			errn = avfilter_graph_parse(graph_.get(), yadif_ss.str().c_str(), inputs, outputs, NULL);
			if(errn < 0)
			{
				BOOST_THROW_EXCEPTION(caspar_exception() <<	msg_info(av_error_str(errn)) <<
					boost::errinfo_api_function("avfilter_graph_parse") << boost::errinfo_errno(AVUNERROR(errn)));
			}

//			av_free(outputs);
//			av_free(inputs);

			errn = avfilter_graph_config(graph_.get(), NULL);
			if(errn < 0)
			{
				BOOST_THROW_EXCEPTION(caspar_exception() <<	msg_info(av_error_str(errn)) 
					<<	boost::errinfo_api_function("avfilter_graph_config") <<	boost::errinfo_errno(AVUNERROR(errn)));
			}
		}
	
		errn = av_vsrc_buffer_add_frame(video_in_filter_, frame.get(), 0);
		if(errn < 0)
		{
			BOOST_THROW_EXCEPTION(caspar_exception() << msg_info(av_error_str(errn)) <<
				boost::errinfo_api_function("av_vsrc_buffer_add_frame") << boost::errinfo_errno(AVUNERROR(errn)));
		}

		errn = avfilter_poll_frame(video_out_filter_->inputs[0]);
		if(errn < 0)
		{
			BOOST_THROW_EXCEPTION(caspar_exception() <<	msg_info(av_error_str(errn)) <<
				boost::errinfo_api_function("avfilter_poll_frame") << boost::errinfo_errno(AVUNERROR(errn)));
		}

		std::generate_n(std::back_inserter(result), errn, [&]{return get_frame(video_out_filter_->inputs[0]);});

		return result;
	}
		
	safe_ptr<core::write_frame> get_frame(AVFilterLink* link)
	{		
		int errn = avfilter_request_frame(link); 			
		if(errn < 0)
		{
			BOOST_THROW_EXCEPTION(caspar_exception() <<	msg_info(av_error_str(errn)) <<
				boost::errinfo_api_function("avfilter_request_frame") << boost::errinfo_errno(AVUNERROR(errn)));
		}
		
		auto buf   = link->cur_buf->buf;
		auto pic   = reinterpret_cast<AVPicture*>(link->cur_buf->buf);
		auto desc  = get_pixel_format_desc(static_cast<PixelFormat>(buf->format), link->w, link->h);
		auto write = frame_factory_->create_frame(this, desc);

		tbb::parallel_for(0, static_cast<int>(desc.planes.size()), 1, [&](int n)
		{
			auto plane            = desc.planes[n];
			auto result           = write->image_data(n).begin();
			auto decoded          = pic->data[n];
			auto decoded_linesize = pic->linesize[n];
				
			// Copy line by line since ffmpeg sometimes pads each line.
			tbb::parallel_for(tbb::blocked_range<size_t>(0, static_cast<int>(desc.planes[n].height)), [&](const tbb::blocked_range<size_t>& r)
			{
				for(size_t y = r.begin(); y != r.end(); ++y)
					memcpy(result + y*plane.linesize, decoded + y*decoded_linesize, plane.linesize);
			});

			write->commit(n);
		});
			
		return write;
	}
};

deinterlacer::deinterlacer(const safe_ptr<core::frame_factory>& factory) : impl_(implementation(factory)) {}
std::vector<safe_ptr<core::write_frame>> deinterlacer::execute(const safe_ptr<AVFrame>& frame) {return impl_->execute(frame);}

}