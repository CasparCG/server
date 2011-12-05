/*
* Copyright (c) 2011 Sveriges Television AB <info@casparcg.com>
*
* This file is part of CasparCG (www.casparcg.com).
*
* CasparCG is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* CasparCG is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with CasparCG. If not, see <http://www.gnu.org/licenses/>.
*
* Author: Robert Nagy, ronag89@gmail.com
*/

#include "../../stdafx.h"

#include "filter.h"

#include "parallel_yadif.h"

#include "../../ffmpeg_error.h"

#include <common/exception/exceptions.h>

#include <boost/assign.hpp>
#include <boost/range/iterator_range.hpp>
#include <boost/range/adaptors.hpp>
#include <boost/assign.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/foreach.hpp>

#include <cstdio>
#include <sstream>

#if defined(_MSC_VER)
#pragma warning (push)
#pragma warning (disable : 4244)
#endif
extern "C" 
{
	#include <libavutil/avutil.h>
	#include <libavutil/imgutils.h>
	#include <libavfilter/avfilter.h>
	#include <libavfilter/avcodec.h>
	#include <libavfilter/avfiltergraph.h>
	#include <libavfilter/buffersink.h>
	#include <libavfilter/vsrc_buffer.h>
}
#if defined(_MSC_VER)
#pragma warning (pop)
#endif


namespace caspar { namespace ffmpeg {

static int query_formats_444(AVFilterContext *ctx)
{
    static const int pix_fmts[] = {PIX_FMT_YUV444P, PIX_FMT_NONE};
    avfilter_set_common_pixel_formats(ctx, avfilter_make_format_list(pix_fmts));
    return 0;
}

static int query_formats_422(AVFilterContext *ctx)
{
    static const int pix_fmts[] = {PIX_FMT_YUV422P, PIX_FMT_NONE};
    avfilter_set_common_pixel_formats(ctx, avfilter_make_format_list(pix_fmts));
    return 0;
}

static int query_formats_420(AVFilterContext *ctx)
{
    static const int pix_fmts[] = {PIX_FMT_YUV420P, PIX_FMT_NONE};
    avfilter_set_common_pixel_formats(ctx, avfilter_make_format_list(pix_fmts));
    return 0;
}

static int query_formats_420a(AVFilterContext *ctx)
{
    static const int pix_fmts[] = {PIX_FMT_YUVA420P, PIX_FMT_NONE};
    avfilter_set_common_pixel_formats(ctx, avfilter_make_format_list(pix_fmts));
    return 0;
}

static int query_formats_411(AVFilterContext *ctx)
{
    static const int pix_fmts[] = {PIX_FMT_YUV411P, PIX_FMT_NONE};
    avfilter_set_common_pixel_formats(ctx, avfilter_make_format_list(pix_fmts));
    return 0;
}

static int query_formats_410(AVFilterContext *ctx)
{
    static const int pix_fmts[] = {PIX_FMT_YUV410P, PIX_FMT_NONE};
    avfilter_set_common_pixel_formats(ctx, avfilter_make_format_list(pix_fmts));
    return 0;
}

struct filter::implementation
{
	std::wstring					filters_;
	std::shared_ptr<AVFilterGraph>	graph_;	
	AVFilterContext*				buffersink_ctx_;
	AVFilterContext*				buffersrc_ctx_;
	std::shared_ptr<void>			parallel_yadif_ctx_;
	std::vector<PixelFormat>		pix_fmts_;
	std::queue<safe_ptr<AVFrame>>	bypass_;
		
	implementation(const std::wstring& filters, const std::vector<PixelFormat>& pix_fmts) 
		: filters_(filters)
		, parallel_yadif_ctx_(nullptr)
		, pix_fmts_(pix_fmts)
	{
		if(pix_fmts_.empty())
		{
			pix_fmts_ = boost::assign::list_of
				(PIX_FMT_YUVA420P)
				(PIX_FMT_YUV444P)
				(PIX_FMT_YUV422P)
				(PIX_FMT_YUV420P)
				(PIX_FMT_YUV411P)
				(PIX_FMT_BGRA)
				(PIX_FMT_ARGB)
				(PIX_FMT_RGBA)
				(PIX_FMT_ABGR)
				(PIX_FMT_GRAY8);
		}
		
		pix_fmts_.push_back(PIX_FMT_NONE);
	}
	
	void push(const std::shared_ptr<AVFrame>& frame)
	{		
		if(!frame)
			return;

		if(frame->data[0] == nullptr || frame->width < 1)
			BOOST_THROW_EXCEPTION(invalid_argument());

		if(filters_.empty())
		{
			bypass_.push(make_safe_ptr(frame));
			return;
		}
		
		try
		{
			if(!graph_)
			{
				try
				{
					graph_.reset(avfilter_graph_alloc(), [](AVFilterGraph* p){avfilter_graph_free(&p);});
								
					// Input
					std::stringstream args;
					args << frame->width << ":" << frame->height << ":" << frame->format << ":" << 0 << ":" << 0 << ":" << 0 << ":" << 0; // don't care about pts and aspect_ratio
					THROW_ON_ERROR2(avfilter_graph_create_filter(&buffersrc_ctx_, avfilter_get_by_name("buffer"), "src", args.str().c_str(), NULL, graph_.get()), "[filter]");
					
#if FF_API_OLD_VSINK_API
					THROW_ON_ERROR2(avfilter_graph_create_filter(&buffersink_ctx_, avfilter_get_by_name("buffersink"), "out", NULL, pix_fmts_.data(), graph_.get()), "[filter]");
#else
					safe_ptr<AVBufferSinkParams> buffersink_params(av_buffersink_params_alloc(), av_free);
					buffersink_params->pixel_fmts = pix_fmts_.data();
					THROW_ON_ERROR2(avfilter_graph_create_filter(&buffersink_ctx_, avfilter_get_by_name("buffersink"), "out", NULL, buffersink_params.get(), graph_.get()), "[filter]");
#endif
					AVFilterInOut* inputs  = avfilter_inout_alloc();
					AVFilterInOut* outputs = avfilter_inout_alloc();
								
					outputs->name			= av_strdup("in");
					outputs->filter_ctx		= buffersrc_ctx_;
					outputs->pad_idx		= 0;
					outputs->next			= nullptr;

					inputs->name			= av_strdup("out");
					inputs->filter_ctx		= buffersink_ctx_;
					inputs->pad_idx			= 0;
					inputs->next			= nullptr;
			
					std::string filters = boost::to_lower_copy(narrow(filters_));
					THROW_ON_ERROR2(avfilter_graph_parse(graph_.get(), filters.c_str(), &inputs, &outputs, NULL), "[filter]");
			
					auto yadif_filter = boost::adaptors::filtered([&](AVFilterContext* p){return strstr(p->name, "yadif") != 0;});

					BOOST_FOREACH(auto filter_ctx, boost::make_iterator_range(graph_->filters, graph_->filters + graph_->filter_count) | yadif_filter)
					{
						// Don't trust that libavfilter chooses optimal format.
						filter_ctx->filter->query_formats = [&]() -> int (*)(AVFilterContext*)
						{
							switch(frame->format)
							{
							case PIX_FMT_YUV444P16: 
							case PIX_FMT_YUV444P10: 
							case PIX_FMT_YUV444P9:  	
							case PIX_FMT_YUV444P:	
							case PIX_FMT_BGR24:		
							case PIX_FMT_RGB24:	
								return query_formats_444;
							case PIX_FMT_YUV422P16: 
							case PIX_FMT_YUV422P10: 
							case PIX_FMT_YUV422P9:  
							case PIX_FMT_YUV422P:	
							case PIX_FMT_UYVY422:	
							case PIX_FMT_YUYV422:	
								return query_formats_422;
							case PIX_FMT_YUV420P16: 
							case PIX_FMT_YUV420P10: 
							case PIX_FMT_YUV420P9:  
							case PIX_FMT_YUV420P:	
								return query_formats_420;
							case PIX_FMT_YUVA420P:	
							case PIX_FMT_BGRA:		
							case PIX_FMT_RGBA:		
							case PIX_FMT_ABGR:		
							case PIX_FMT_ARGB:		
								return query_formats_420a;
							case PIX_FMT_UYYVYY411: 
							case PIX_FMT_YUV411P:	
								return query_formats_411;
							case PIX_FMT_YUV410P:	
								return query_formats_410;
							default:				
								return filter_ctx->filter->query_formats;
							}
						}();
					}
					
					THROW_ON_ERROR2(avfilter_graph_config(graph_.get(), NULL), "[filter]");	
					
					BOOST_FOREACH(auto filter_ctx, boost::make_iterator_range(graph_->filters, graph_->filters + graph_->filter_count) | yadif_filter)						
						parallel_yadif_ctx_ = make_parallel_yadif(filter_ctx);						
				}
				catch(...)
				{
					graph_ = nullptr;
					throw;
				}
			}
		
			THROW_ON_ERROR2(av_vsrc_buffer_add_frame(buffersrc_ctx_, frame.get(), 0), "[filter]");
		}
		catch(ffmpeg_error&)
		{
			throw;
		}
		catch(...)
		{
			BOOST_THROW_EXCEPTION(ffmpeg_error() << boost::errinfo_nested_exception(boost::current_exception()));
		}
	}

	std::shared_ptr<AVFrame> poll()
	{
		if(filters_.empty())
		{
			if(bypass_.empty())
				return nullptr;
			auto frame = bypass_.front();
			bypass_.pop();
			return frame;
		}

		if(!graph_)
			return nullptr;
		
		try
		{
			if(avfilter_poll_frame(buffersink_ctx_->inputs[0])) 
			{
				AVFilterBufferRef *picref;
				THROW_ON_ERROR2(av_buffersink_get_buffer_ref(buffersink_ctx_, &picref, 0), "[filter]");

				if (!picref) 
					return nullptr;
				
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
					
				return frame;				
			}

			return nullptr;
		}
		catch(ffmpeg_error&)
		{
			throw;
		}
		catch(...)
		{
			BOOST_THROW_EXCEPTION(ffmpeg_error() << boost::errinfo_nested_exception(boost::current_exception()));
		}
	}
};

filter::filter(const std::wstring& filters, const std::vector<PixelFormat>& pix_fmts) : impl_(new implementation(filters, pix_fmts)){}
filter::filter(filter&& other) : impl_(std::move(other.impl_)){}
filter& filter::operator=(filter&& other){impl_ = std::move(other.impl_); return *this;}
void filter::push(const std::shared_ptr<AVFrame>& frame){impl_->push(frame);}
std::shared_ptr<AVFrame> filter::poll(){return impl_->poll();}
std::wstring filter::filter_str() const{return impl_->filters_;}
std::vector<safe_ptr<AVFrame>> filter::poll_all()
{	
	std::vector<safe_ptr<AVFrame>> frames;
	for(auto frame = poll(); frame; frame = poll())
		frames.push_back(make_safe_ptr(frame));
	return frames;
}

}}