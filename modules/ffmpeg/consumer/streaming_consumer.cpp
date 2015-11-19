#include "../StdAfx.h"

#include "ffmpeg_consumer.h"

#include "../ffmpeg_error.h"

#include <common/except.h>
#include <common/executor.h>
#include <common/assert.h>
#include <common/utf.h>
#include <common/future.h>
#include <common/env.h>
#include <common/scope_exit.h>

#include <core/consumer/frame_consumer.h>
#include <core/frame/frame.h>
#include <core/frame/audio_channel_layout.h>
#include <core/video_format.h>
#include <core/monitor/monitor.h>
#include <core/help/help_repository.h>
#include <core/help/help_sink.h>

#include <boost/noncopyable.hpp>
#include <boost/rational.hpp>
#include <boost/format.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/property_tree/ptree.hpp>

#pragma warning(push)
#pragma warning(disable: 4244)
#pragma warning(disable: 4245)
#include <boost/crc.hpp>
#pragma warning(pop)

#include <tbb/atomic.h>
#include <tbb/concurrent_queue.h>
#include <tbb/parallel_invoke.h>
#include <tbb/parallel_for.h>

#include <numeric>

#pragma warning(push)
#pragma warning(disable: 4244)

extern "C" 
{
	#define __STDC_CONSTANT_MACROS
	#define __STDC_LIMIT_MACROS
	#include <libavformat/avformat.h>
	#include <libavcodec/avcodec.h>
	#include <libavutil/avutil.h>
	#include <libavutil/frame.h>
	#include <libavutil/opt.h>
	#include <libavutil/imgutils.h>
	#include <libavutil/parseutils.h>
	#include <libavfilter/avfilter.h>
	#include <libavfilter/buffersink.h>
	#include <libavfilter/buffersrc.h>
}

#pragma warning(pop)

namespace caspar { namespace ffmpeg {

int crc16(const std::string& str)
{
	boost::crc_16_type result;

	result.process_bytes(str.data(), str.length());

	return result.checksum();
}

class streaming_consumer final : public core::frame_consumer
{
public:
	// Static Members
		
private:
	core::monitor::subject						subject_;
	boost::filesystem::path						path_;
	int											consumer_index_offset_;

	std::map<std::string, std::string>			options_;
	bool										compatibility_mode_;
												
	core::video_format_desc						in_video_format_;
	core::audio_channel_layout					in_channel_layout_			= core::audio_channel_layout::invalid();

	std::shared_ptr<AVFormatContext>			oc_;
	tbb::atomic<bool>							abort_request_;
												
	std::shared_ptr<AVStream>					video_st_;
	std::shared_ptr<AVStream>					audio_st_;

	std::int64_t								video_pts_;
	std::int64_t								audio_pts_;
																					
    AVFilterContext*							audio_graph_in_;  
    AVFilterContext*							audio_graph_out_; 
    std::shared_ptr<AVFilterGraph>				audio_graph_;    
	std::shared_ptr<AVBitStreamFilterContext>	audio_bitstream_filter_;       

    AVFilterContext*							video_graph_in_;  
    AVFilterContext*							video_graph_out_; 
    std::shared_ptr<AVFilterGraph>				video_graph_;  
	std::shared_ptr<AVBitStreamFilterContext>	video_bitstream_filter_;
	
	executor									executor_;

	executor									video_encoder_executor_;
	executor									audio_encoder_executor_;

	tbb::atomic<int>							tokens_;
	boost::mutex								tokens_mutex_;
	boost::condition_variable					tokens_cond_;
	tbb::atomic<int64_t>						current_encoding_delay_;

	executor									write_executor_;
	
public:

	streaming_consumer(
			std::string path, 
			std::string options,
			bool compatibility_mode)
		: path_(path)
		, consumer_index_offset_(crc16(path))
		, compatibility_mode_(compatibility_mode)
		, video_pts_(0)
		, audio_pts_(0)
		, executor_(print())
		, audio_encoder_executor_(print() + L" audio_encoder")
		, video_encoder_executor_(print() + L" video_encoder")
		, write_executor_(print() + L" io")
	{		
		abort_request_ = false;
		current_encoding_delay_ = 0;

		for(auto it = 
				boost::sregex_iterator(
					options.begin(), 
					options.end(), 
					boost::regex("-(?<NAME>[^-\\s]+)(\\s+(?<VALUE>[^\\s]+))?")); 
			it != boost::sregex_iterator(); 
			++it)
		{				
			options_[(*it)["NAME"].str()] = (*it)["VALUE"].matched ? (*it)["VALUE"].str() : "";
		}
										
        if (options_.find("threads") == options_.end())
            options_["threads"] = "auto";

		tokens_ = 
			std::max(
				1, 
				try_remove_arg<int>(
					options_, 
					boost::regex("tokens")).get_value_or(2));		
	}
		
	~streaming_consumer()
	{
		if(oc_)
		{
			video_encoder_executor_.begin_invoke([&] { encode_video(core::const_frame::empty(), nullptr); });
			audio_encoder_executor_.begin_invoke([&] { encode_audio(core::const_frame::empty(), nullptr); });

			video_encoder_executor_.stop();
			audio_encoder_executor_.stop();
			video_encoder_executor_.join();
			audio_encoder_executor_.join();

			video_graph_.reset();
			audio_graph_.reset();
			video_st_.reset();
			audio_st_.reset();

			write_packet(nullptr, nullptr);

			write_executor_.stop();
			write_executor_.join();

			FF(av_write_trailer(oc_.get()));

			if (!(oc_->oformat->flags & AVFMT_NOFILE) && oc_->pb)
				avio_close(oc_->pb);

			oc_.reset();
		}
	}

	void initialize(
			const core::video_format_desc& format_desc,
			const core::audio_channel_layout& channel_layout,
			int channel_index) override
	{
		try
		{				
			static boost::regex prot_exp("^.+:.*" );
			
			const auto overwrite = 
				try_remove_arg<std::string>(
					options_,
					boost::regex("y")) != boost::none;

			if(!boost::regex_match(
					path_.string(), 
					prot_exp))
			{
				if(!path_.is_complete())
				{
					path_ = 
						u8(
							env::media_folder()) + 
							path_.string();
				}
			
				if(boost::filesystem::exists(path_))
				{
					if(!overwrite && !compatibility_mode_)
						BOOST_THROW_EXCEPTION(invalid_argument() << msg_info("File exists"));
						
					boost::filesystem::remove(path_);
				}
			}
							
			const auto oformat_name = 
				try_remove_arg<std::string>(
					options_, 
					boost::regex("^f|format$"));
			
			AVFormatContext* oc;

			FF(avformat_alloc_output_context2(
				&oc, 
				nullptr, 
				oformat_name && !oformat_name->empty() ? oformat_name->c_str() : nullptr, 
				path_.string().c_str()));

			oc_.reset(
				oc, 
				avformat_free_context);
					
			CASPAR_VERIFY(oc_->oformat);

			oc_->interrupt_callback.callback = streaming_consumer::interrupt_cb;
			oc_->interrupt_callback.opaque   = this;	

			CASPAR_VERIFY(format_desc.format != core::video_format::invalid);

			in_video_format_ = format_desc;
			in_channel_layout_ = channel_layout;
							
			CASPAR_VERIFY(oc_->oformat);
			
			const auto video_codec_name = 
				try_remove_arg<std::string>(
					options_, 
					boost::regex("^c:v|codec:v|vcodec$"));

			const auto video_codec = 
				video_codec_name 
					? avcodec_find_encoder_by_name(video_codec_name->c_str())
					: avcodec_find_encoder(oc_->oformat->video_codec);
						
			const auto audio_codec_name = 
				try_remove_arg<std::string>(
					options_, 
					 boost::regex("^c:a|codec:a|acodec$"));
			
			const auto audio_codec = 
				audio_codec_name 
					? avcodec_find_encoder_by_name(audio_codec_name->c_str())
					: avcodec_find_encoder(oc_->oformat->audio_codec);
			
			if (!video_codec)
				CASPAR_THROW_EXCEPTION(user_error() << msg_info(
						"Failed to find video codec " + (video_codec_name
								? *video_codec_name
								: "with id " + boost::lexical_cast<std::string>(
										oc_->oformat->video_codec))));
			if (!audio_codec)
				CASPAR_THROW_EXCEPTION(user_error() << msg_info(
						"Failed to find audio codec " + (audio_codec_name
								? *audio_codec_name
								: "with id " + boost::lexical_cast<std::string>(
										oc_->oformat->audio_codec))));
			
			// Filters

			{
				configure_video_filters(
					*video_codec, 
					try_remove_arg<std::string>(options_, 
					boost::regex("vf|f:v|filter:v")).get_value_or(""));

				configure_audio_filters(
					*audio_codec, 
					try_remove_arg<std::string>(options_,
					boost::regex("af|f:a|filter:a")).get_value_or(""));
			}

			// Bistream Filters
			{
				configue_audio_bistream_filters(options_);
				configue_video_bistream_filters(options_);
			}

			// Encoders

			{
				auto video_options = options_;
				auto audio_options = options_;

				video_st_ = open_encoder(
					*video_codec, 
					video_options);

				audio_st_ = open_encoder(
					*audio_codec, 
					audio_options);

				auto it = options_.begin();
				while(it != options_.end())
				{
					if(video_options.find(it->first) == video_options.end() || audio_options.find(it->first) == audio_options.end())
						it = options_.erase(it);
					else
						++it;
				}
			}

			// Output
			{
				AVDictionary* av_opts = nullptr;

				to_dict(
					&av_opts, 
					std::move(options_));

				CASPAR_SCOPE_EXIT
				{
					av_dict_free(&av_opts);
				};

				if (!(oc_->oformat->flags & AVFMT_NOFILE)) 
				{
					FF(avio_open2(
						&oc_->pb, 
						path_.string().c_str(), 
						AVIO_FLAG_WRITE, 
						&oc_->interrupt_callback, 
						&av_opts));
				}
				
				FF(avformat_write_header(
					oc_.get(), 
					&av_opts));
				
				options_ = to_map(av_opts);
			}

			// Dump Info
			
			av_dump_format(
				oc_.get(), 
				0, 
				oc_->filename, 
				1);		

			for (const auto& option : options_)
			{
				CASPAR_LOG(warning) 
					<< L"Invalid option: -" 
					<< u16(option.first) 
					<< L" " 
					<< u16(option.second);
			}
		}
		catch(...)
		{
			video_st_.reset();
			audio_st_.reset();
			oc_.reset();
			throw;
		}
	}

	core::monitor::subject& monitor_output() override
	{
		return subject_;
	}

	std::wstring name() const override
	{
		return L"streaming";
	}

	std::future<bool> send(core::const_frame frame) override
	{		
		CASPAR_VERIFY(in_video_format_.format != core::video_format::invalid);
		
		--tokens_;
		std::shared_ptr<void> token(
			nullptr, 
			[this, frame](void*)
			{
				++tokens_;
				tokens_cond_.notify_one();
				current_encoding_delay_ = frame.get_age_millis();
			});

		return executor_.begin_invoke([=]() -> bool
		{
			boost::unique_lock<boost::mutex> tokens_lock(tokens_mutex_);

			while(tokens_ < 0)
				tokens_cond_.wait(tokens_lock);

			video_encoder_executor_.begin_invoke([=]() mutable
			{
				encode_video(
					frame, 
					token);
			});
		
			audio_encoder_executor_.begin_invoke([=]() mutable
			{
				encode_audio(
					frame, 
					token);
			});
				
			return true;
		});
	}

	std::wstring print() const override
	{
		return L"streaming_consumer[" + u16(path_.string()) + L"]";
	}
	
	virtual boost::property_tree::wptree info() const override
	{
		boost::property_tree::wptree info;
		info.add(L"type", L"stream");
		info.add(L"path", path_.wstring());
		return info;
	}

	bool has_synchronization_clock() const override
	{
		return false;
	}

	int buffer_depth() const override
	{
		return -1;
	}

	int index() const override
	{
		return compatibility_mode_ ? 200 : 100000 + consumer_index_offset_;
	}

	int64_t presentation_frame_age_millis() const override
	{
		return current_encoding_delay_;
	}

private:

	static int interrupt_cb(void* ctx)
	{
		CASPAR_ASSERT(ctx);
		return reinterpret_cast<streaming_consumer*>(ctx)->abort_request_;		
	}
		
	std::shared_ptr<AVStream> open_encoder(
			const AVCodec& codec,
			std::map<std::string,
			std::string>& options)
	{			
		auto st = 
			avformat_new_stream(
				oc_.get(), 
				&codec);

		if (!st) 		
			CASPAR_THROW_EXCEPTION(caspar_exception() << msg_info("Could not allocate video-stream.") << boost::errinfo_api_function("av_new_stream"));

		auto enc = st->codec;
				
		CASPAR_VERIFY(enc);
						
		switch(enc->codec_type)
		{
			case AVMEDIA_TYPE_VIDEO:
			{
				enc->time_base				= video_graph_out_->inputs[0]->time_base;
				enc->pix_fmt				= static_cast<AVPixelFormat>(video_graph_out_->inputs[0]->format);
				enc->sample_aspect_ratio	= st->sample_aspect_ratio = video_graph_out_->inputs[0]->sample_aspect_ratio;
				enc->width					= video_graph_out_->inputs[0]->w;
				enc->height					= video_graph_out_->inputs[0]->h;
				enc->bit_rate_tolerance		= 400 * 1000000;
			
				break;
			}
			case AVMEDIA_TYPE_AUDIO:
			{
				enc->time_base				= audio_graph_out_->inputs[0]->time_base;
				enc->sample_fmt				= static_cast<AVSampleFormat>(audio_graph_out_->inputs[0]->format);
				enc->sample_rate			= audio_graph_out_->inputs[0]->sample_rate;
				enc->channel_layout			= audio_graph_out_->inputs[0]->channel_layout;
				enc->channels				= audio_graph_out_->inputs[0]->channels;
			
				break;
			}
		}
										
		if(oc_->oformat->flags & AVFMT_GLOBALHEADER)
			enc->flags |= CODEC_FLAG_GLOBAL_HEADER;
		
		static const std::array<std::string, 4> char_id_map = {{"v", "a", "d", "s"}};

		const auto char_id = char_id_map.at(enc->codec_type);
								
		const auto codec_opts = 
			remove_options(
				options, 
				boost::regex("^(" + char_id + "?[^:]+):" + char_id + "$"));
		
		AVDictionary* av_codec_opts = nullptr;

		to_dict(
			&av_codec_opts, 
			options);

		to_dict(
			&av_codec_opts,
			codec_opts);

		options.clear();
		
		FF(avcodec_open2(
			enc,		
			&codec, 
			av_codec_opts ? &av_codec_opts : nullptr));		

		if(av_codec_opts)
		{
			auto t = 
				av_dict_get(
					av_codec_opts, 
					"", 
					 nullptr, 
					AV_DICT_IGNORE_SUFFIX);

			while(t)
			{
				options[t->key + (codec_opts.find(t->key) != codec_opts.end() ? ":" + char_id : "")] = t->value;

				t = av_dict_get(
						av_codec_opts, 
						"", 
						t, 
						AV_DICT_IGNORE_SUFFIX);
			}

			av_dict_free(&av_codec_opts);
		}
				
		if(enc->codec_type == AVMEDIA_TYPE_AUDIO && !(codec.capabilities & CODEC_CAP_VARIABLE_FRAME_SIZE))
		{
			CASPAR_ASSERT(enc->frame_size > 0);
			av_buffersink_set_frame_size(audio_graph_out_, 
										 enc->frame_size);
		}
		
		return std::shared_ptr<AVStream>(st, [this](AVStream* st)
		{
			avcodec_close(st->codec);
		});
	}

	void configue_audio_bistream_filters(
			std::map<std::string, std::string>& options)
	{
		const auto audio_bitstream_filter_str = 
			try_remove_arg<std::string>(
				options, 
				boost::regex("^bsf:a|absf$"));

		const auto audio_bitstream_filter = 
			audio_bitstream_filter_str 
				? av_bitstream_filter_init(audio_bitstream_filter_str->c_str()) 
				: nullptr;

		CASPAR_VERIFY(!audio_bitstream_filter_str || audio_bitstream_filter);

		if(audio_bitstream_filter)
		{
			audio_bitstream_filter_.reset(
				audio_bitstream_filter, 
				av_bitstream_filter_close);
		}
		
		if(audio_bitstream_filter_str && !audio_bitstream_filter_)
			options["bsf:a"] = *audio_bitstream_filter_str;
	}
	
	void configue_video_bistream_filters(
			std::map<std::string, std::string>& options)
	{
		const auto video_bitstream_filter_str = 
				try_remove_arg<std::string>(
					options, 
					boost::regex("^bsf:v|vbsf$"));

		const auto video_bitstream_filter = 
			video_bitstream_filter_str 
				? av_bitstream_filter_init(video_bitstream_filter_str->c_str()) 
				: nullptr;

		CASPAR_VERIFY(!video_bitstream_filter_str || video_bitstream_filter);

		if(video_bitstream_filter)
		{
			video_bitstream_filter_.reset(
				video_bitstream_filter, 
				av_bitstream_filter_close);
		}
		
		if(video_bitstream_filter_str && !video_bitstream_filter_)
			options["bsf:v"] = *video_bitstream_filter_str;
	}
	
	void configure_video_filters(
			const AVCodec& codec,
			const std::string& filtergraph)
	{
		video_graph_.reset(
				avfilter_graph_alloc(), 
				[](AVFilterGraph* p)
				{
					avfilter_graph_free(&p);
				});
		
		video_graph_->nb_threads  = boost::thread::hardware_concurrency()/2;
		video_graph_->thread_type = AVFILTER_THREAD_SLICE;

		const auto sample_aspect_ratio =
			boost::rational<int>(
					in_video_format_.square_width,
					in_video_format_.square_height) /
			boost::rational<int>(
					in_video_format_.width,
					in_video_format_.height);
		
		const auto vsrc_options = (boost::format("video_size=%1%x%2%:pix_fmt=%3%:time_base=%4%/%5%:pixel_aspect=%6%/%7%:frame_rate=%8%/%9%")
			% in_video_format_.width % in_video_format_.height
			% AV_PIX_FMT_BGRA
			% in_video_format_.duration	% in_video_format_.time_scale
			% sample_aspect_ratio.numerator() % sample_aspect_ratio.denominator()
			% in_video_format_.time_scale % in_video_format_.duration).str();
					
		AVFilterContext* filt_vsrc = nullptr;			
		FF(avfilter_graph_create_filter(
				&filt_vsrc,
				avfilter_get_by_name("buffer"), 
				"ffmpeg_consumer_buffer",
				vsrc_options.c_str(), 
				nullptr, 
				video_graph_.get()));
				
		AVFilterContext* filt_vsink = nullptr;
		FF(avfilter_graph_create_filter(
				&filt_vsink,
				avfilter_get_by_name("buffersink"), 
				"ffmpeg_consumer_buffersink",
				nullptr, 
				nullptr, 
				video_graph_.get()));
		
#pragma warning (push)
#pragma warning (disable : 4245)

		FF(av_opt_set_int_list(
				filt_vsink, 
				"pix_fmts", 
				codec.pix_fmts, 
				-1,
				AV_OPT_SEARCH_CHILDREN));

#pragma warning (pop)
			
		configure_filtergraph(
				*video_graph_, 
				filtergraph,
				*filt_vsrc,
				*filt_vsink);

		video_graph_in_  = filt_vsrc;
		video_graph_out_ = filt_vsink;
		
		CASPAR_LOG(info)
			<< 	u16(std::string("\n") 
				+ avfilter_graph_dump(
						video_graph_.get(), 
						nullptr));
	}

	void configure_audio_filters(
			const AVCodec& codec,
			const std::string& filtergraph)
	{
		audio_graph_.reset(
			avfilter_graph_alloc(), 
			[](AVFilterGraph* p)
			{
				avfilter_graph_free(&p);
			});
		
		audio_graph_->nb_threads  = boost::thread::hardware_concurrency()/2;
		audio_graph_->thread_type = AVFILTER_THREAD_SLICE;
		
		const auto asrc_options = (boost::format("sample_rate=%1%:sample_fmt=%2%:channels=%3%:time_base=%4%/%5%:channel_layout=%6%")
			% in_video_format_.audio_sample_rate
			% av_get_sample_fmt_name(AV_SAMPLE_FMT_S32)
			% in_channel_layout_.num_channels
			% 1	% in_video_format_.audio_sample_rate
			% boost::io::group(
				std::hex, 
				std::showbase, 
				av_get_default_channel_layout(in_channel_layout_.num_channels))).str();

		AVFilterContext* filt_asrc = nullptr;
		FF(avfilter_graph_create_filter(
			&filt_asrc,
			avfilter_get_by_name("abuffer"), 
			"ffmpeg_consumer_abuffer",
			asrc_options.c_str(), 
			nullptr, 
			audio_graph_.get()));
				
		AVFilterContext* filt_asink = nullptr;
		FF(avfilter_graph_create_filter(
			&filt_asink,
			avfilter_get_by_name("abuffersink"), 
			"ffmpeg_consumer_abuffersink",
			nullptr, 
			nullptr, 
			audio_graph_.get()));
		
#pragma warning (push)
#pragma warning (disable : 4245)

		FF(av_opt_set_int(
			filt_asink,	   
			"all_channel_counts",
			1,	
			AV_OPT_SEARCH_CHILDREN));

		FF(av_opt_set_int_list(
			filt_asink, 
			"sample_fmts",		 
			codec.sample_fmts,				
			-1, 
			AV_OPT_SEARCH_CHILDREN));

		FF(av_opt_set_int_list(
			filt_asink,
			"channel_layouts",	 
			codec.channel_layouts,			
			-1, 
			AV_OPT_SEARCH_CHILDREN));

		FF(av_opt_set_int_list(
			filt_asink, 
			"sample_rates" ,	 
			codec.supported_samplerates,	
			-1, 
			AV_OPT_SEARCH_CHILDREN));

#pragma warning (pop)
			
		configure_filtergraph(
			*audio_graph_, 
			filtergraph, 
			*filt_asrc, 
			*filt_asink);

		audio_graph_in_  = filt_asrc;
		audio_graph_out_ = filt_asink;

		CASPAR_LOG(info) 
			<< 	u16(std::string("\n") 
				+ avfilter_graph_dump(
					audio_graph_.get(), 
					nullptr));
	}

	void configure_filtergraph(
			AVFilterGraph& graph,
			const std::string& filtergraph,
			AVFilterContext& source_ctx,
			AVFilterContext& sink_ctx)
	{
		AVFilterInOut* outputs = nullptr;
		AVFilterInOut* inputs = nullptr;

		try
		{
			if(!filtergraph.empty())
			{
				outputs = avfilter_inout_alloc();
				inputs  = avfilter_inout_alloc();

				CASPAR_VERIFY(outputs && inputs);

				outputs->name       = av_strdup("in");
				outputs->filter_ctx = &source_ctx;
				outputs->pad_idx    = 0;
				outputs->next       = nullptr;

				inputs->name        = av_strdup("out");
				inputs->filter_ctx  = &sink_ctx;
				inputs->pad_idx     = 0;
				inputs->next        = nullptr;

				FF(avfilter_graph_parse(
					&graph, 
					filtergraph.c_str(), 
					inputs,
					outputs,
					nullptr));
			} 
			else 
			{
				FF(avfilter_link(
					&source_ctx, 
					0, 
					&sink_ctx, 
					0));
			}

			FF(avfilter_graph_config(
				&graph, 
				nullptr));
		}
		catch(...)
		{
			avfilter_inout_free(&outputs);
			avfilter_inout_free(&inputs);
			throw;
		}
	}
	
	void encode_video(core::const_frame frame_ptr, std::shared_ptr<void> token)
	{		
		if(!video_st_)
			return;

		auto enc = video_st_->codec;
			
		std::shared_ptr<AVFrame> src_av_frame;

		if(frame_ptr != core::const_frame::empty())
		{
			src_av_frame.reset(
				av_frame_alloc(),
				[frame_ptr](AVFrame* frame)
				{
					av_frame_free(&frame);
				});

			avcodec_get_frame_defaults(src_av_frame.get());		
			
			const auto sample_aspect_ratio = 
				boost::rational<int>(
					in_video_format_.square_width, 
					in_video_format_.square_height) /
				boost::rational<int>(
					in_video_format_.width, 
					in_video_format_.height);

			src_av_frame->format				  = AV_PIX_FMT_BGRA;
			src_av_frame->width					  = in_video_format_.width;
			src_av_frame->height				  = in_video_format_.height;
			src_av_frame->sample_aspect_ratio.num = sample_aspect_ratio.numerator();
			src_av_frame->sample_aspect_ratio.den = sample_aspect_ratio.denominator();
			src_av_frame->pts					  = video_pts_;

			video_pts_ += 1;

			FF(av_image_fill_arrays(
				src_av_frame->data,
				src_av_frame->linesize,
				frame_ptr.image_data().begin(),
				static_cast<AVPixelFormat>(src_av_frame->format), 
				in_video_format_.width, 
				in_video_format_.height, 
				1));

			FF(av_buffersrc_add_frame(
				video_graph_in_, 
				src_av_frame.get()));
		}		

		int ret = 0;

		while(ret >= 0)
		{
			std::shared_ptr<AVFrame> filt_frame(
				av_frame_alloc(), 
				[](AVFrame* p)
				{
					av_frame_free(&p);
				});

			ret = av_buffersink_get_frame(
				video_graph_out_, 
				filt_frame.get());
						
			video_encoder_executor_.begin_invoke([=]
			{
				if(ret == AVERROR_EOF)
				{
					if(enc->codec->capabilities & CODEC_CAP_DELAY)
					{
						while(encode_av_frame(
								*video_st_, 
								video_bitstream_filter_.get(),
								avcodec_encode_video2, 
								nullptr, token))
						{
							boost::this_thread::yield(); // TODO:
						}
					}		
				}
				else if(ret != AVERROR(EAGAIN))
				{
					FF_RET(ret, "av_buffersink_get_frame");
					
					if (filt_frame->interlaced_frame) 
					{
						if (enc->codec->id == AV_CODEC_ID_MJPEG)
							enc->field_order = filt_frame->top_field_first ? AV_FIELD_TT : AV_FIELD_BB;
						else
							enc->field_order = filt_frame->top_field_first ? AV_FIELD_TB : AV_FIELD_BT;
					} 
					else
						enc->field_order = AV_FIELD_PROGRESSIVE;

					filt_frame->quality = enc->global_quality;

					if (!enc->me_threshold)
						filt_frame->pict_type = AV_PICTURE_TYPE_NONE;
			
					encode_av_frame(
						*video_st_,
						video_bitstream_filter_.get(),
						avcodec_encode_video2,
						filt_frame, 
						token);

					boost::this_thread::yield(); // TODO:
				}
			});
		}
	}
					
	void encode_audio(core::const_frame frame_ptr, std::shared_ptr<void> token)
	{		
		if(!audio_st_)
			return;
		
		auto enc = audio_st_->codec;
			
		std::shared_ptr<AVFrame> src_av_frame;

		if(frame_ptr != core::const_frame::empty())
		{
			src_av_frame.reset(
				av_frame_alloc(), 
				[](AVFrame* p)
				{
					av_frame_free(&p);
				});
		
			src_av_frame->channels		 = in_channel_layout_.num_channels;
			src_av_frame->channel_layout = av_get_default_channel_layout(in_channel_layout_.num_channels);
			src_av_frame->sample_rate	 = in_video_format_.audio_sample_rate;
			src_av_frame->nb_samples	 = static_cast<int>(frame_ptr.audio_data().size()) / src_av_frame->channels;
			src_av_frame->format		 = AV_SAMPLE_FMT_S32;
			src_av_frame->pts			 = audio_pts_;

			audio_pts_ += src_av_frame->nb_samples;

			FF(av_samples_fill_arrays(
					src_av_frame->extended_data,
					src_av_frame->linesize,
					reinterpret_cast<const std::uint8_t*>(&*frame_ptr.audio_data().begin()),
					src_av_frame->channels,
					src_av_frame->nb_samples,
					static_cast<AVSampleFormat>(src_av_frame->format),
					16));
		
			FF(av_buffersrc_add_frame(
					audio_graph_in_, 
					src_av_frame.get()));
		}

		int ret = 0;

		while(ret >= 0)
		{
			std::shared_ptr<AVFrame> filt_frame(
				av_frame_alloc(), 
				[](AVFrame* p)
				{
					av_frame_free(&p);
				});

			ret = av_buffersink_get_frame(
				audio_graph_out_, 
				filt_frame.get());
					
			audio_encoder_executor_.begin_invoke([=]
			{	
				if(ret == AVERROR_EOF)
				{
					if(enc->codec->capabilities & CODEC_CAP_DELAY)
					{
						while(encode_av_frame(
								*audio_st_, 
								audio_bitstream_filter_.get(), 
								avcodec_encode_audio2, 
								nullptr, 
								token))
						{
							boost::this_thread::yield(); // TODO:
						}
					}
				}
				else if(ret != AVERROR(EAGAIN))
				{
					FF_RET(
						ret, 
						"av_buffersink_get_frame");

					encode_av_frame(
						*audio_st_, 
						audio_bitstream_filter_.get(), 
						avcodec_encode_audio2, 
						filt_frame, 
						token);

					boost::this_thread::yield(); // TODO:
				}
			});
		}
	}
	
	template<typename F>
	bool encode_av_frame(
			AVStream& st,
			AVBitStreamFilterContext* bsfc, 
			const F& func, 
			const std::shared_ptr<AVFrame>& src_av_frame, 
			std::shared_ptr<void> token)
	{
		AVPacket pkt = {};
		av_init_packet(&pkt);

		int got_packet = 0;

		FF(func(
			st.codec, 
			&pkt, 
			src_av_frame.get(), 
			&got_packet));
					
		if(!got_packet || pkt.size <= 0)
			return false;

		pkt.stream_index = st.index;
		
		if(bsfc)
		{
			auto new_pkt = pkt;

			auto a = av_bitstream_filter_filter(
					bsfc,
					st.codec,
					nullptr,
					&new_pkt.data,
					&new_pkt.size,
					pkt.data,
					pkt.size,
					pkt.flags & AV_PKT_FLAG_KEY);

			if(a == 0 && new_pkt.data != pkt.data && new_pkt.destruct) 
			{
				auto t = reinterpret_cast<std::uint8_t*>(av_malloc(new_pkt.size + FF_INPUT_BUFFER_PADDING_SIZE));

				if(t) 
				{
					memcpy(
						t, 
						new_pkt.data,
						new_pkt.size);

					memset(
						t + new_pkt.size, 
						0, 
						FF_INPUT_BUFFER_PADDING_SIZE);

					new_pkt.data = t;
					new_pkt.buf  = nullptr;
				} 
				else
					a = AVERROR(ENOMEM);
			}

			av_free_packet(&pkt);

			FF_RET(
				a, 
				"av_bitstream_filter_filter");

			new_pkt.buf =
				av_buffer_create(
					new_pkt.data, 
					new_pkt.size,
					av_buffer_default_free, 
					nullptr, 
					0);

			CASPAR_VERIFY(new_pkt.buf);

			pkt = new_pkt;
		}
		
		if (pkt.pts != AV_NOPTS_VALUE)
		{
			pkt.pts = 
				av_rescale_q(
					pkt.pts,
					st.codec->time_base, 
					st.time_base);
		}

		if (pkt.dts != AV_NOPTS_VALUE)
		{
			pkt.dts = 
				av_rescale_q(
					pkt.dts, 
					st.codec->time_base, 
					st.time_base);
		}
				
		pkt.duration = 
			static_cast<int>(
				av_rescale_q(
					pkt.duration, 
					st.codec->time_base, st.time_base));

		write_packet(
			std::shared_ptr<AVPacket>(
				new AVPacket(pkt), 
				[](AVPacket* p)
				{
					av_free_packet(p); 
					delete p;
				}), token);

		return true;
	}

	void write_packet(
			const std::shared_ptr<AVPacket>& pkt_ptr,
			std::shared_ptr<void> token)
	{		
		write_executor_.begin_invoke([this, pkt_ptr, token]() mutable
		{
			FF(av_interleaved_write_frame(
				oc_.get(), 
				pkt_ptr.get()));
		});	
	}	
	
	template<typename T>
	static boost::optional<T> try_remove_arg(
			std::map<std::string, std::string>& options, 
			const boost::regex& expr)
	{
		for(auto it = options.begin(); it != options.end(); ++it)
		{			
			if(boost::regex_search(it->first, expr))
			{
				auto arg = it->second;
				options.erase(it);
				return boost::lexical_cast<T>(arg);
			}
		}

		return boost::optional<T>();
	}
		
	static std::map<std::string, std::string> remove_options(
			std::map<std::string, std::string>& options, 
			const boost::regex& expr)
	{
		std::map<std::string, std::string> result;
			
		auto it = options.begin();
		while(it != options.end())
		{			
			boost::smatch what;
			if(boost::regex_search(it->first, what, expr))
			{
				result[
					what.size() > 0 && what[1].matched 
						? what[1].str() 
						: it->first] = it->second;
				it = options.erase(it);
			}
			else
				++it;
		}

		return result;
	}
		
	static void to_dict(AVDictionary** dest, const std::map<std::string, std::string>& c)
	{		
		for (const auto& entry : c)
		{
			av_dict_set(
				dest, 
				entry.first.c_str(), 
				entry.second.c_str(), 0);
		}
	}

	static std::map<std::string, std::string> to_map(AVDictionary* dict)
	{
		std::map<std::string, std::string> result;
		
		for(auto t = dict 
				? av_dict_get(
					dict, 
					"", 
					nullptr, 
					AV_DICT_IGNORE_SUFFIX) 
				: nullptr;
			t; 
			t = av_dict_get(
				dict, 
				"", 
				t,
				AV_DICT_IGNORE_SUFFIX))
		{
			result[t->key] = t->value;
		}

		return result;
	}
};

void describe_streaming_consumer(core::help_sink& sink, const core::help_repository& repo)
{
	sink.short_description(L"For streaming the contents of a channel using FFMpeg.");
	sink.syntax(L"STREAM [url:string] {-[ffmpeg_param1:string] [value1:string] {-[ffmpeg_param2:string] [value2:string] {...}}}");
	sink.para()->text(L"For streaming the contents of a channel using FFMpeg");
	sink.definitions()
		->item(L"url", L"The stream URL to create/stream to.")
		->item(L"ffmpeg_paramX", L"A parameter supported by FFMpeg. For example vcodec or acodec etc.");
	sink.para()->text(L"Examples:");
	sink.example(L">> ADD 1 STREAM udp://<client_ip_address>:9250 -format mpegts -vcodec libx264 -crf 25 -tune zerolatency -preset ultrafast");
}

spl::shared_ptr<core::frame_consumer> create_streaming_consumer(
		const std::vector<std::wstring>& params, core::interaction_sink*)
{       
	if (params.size() < 1 || (!boost::iequals(params.at(0), L"STREAM") && !boost::iequals(params.at(0), L"FILE")))
		return core::frame_consumer::empty();

	auto compatibility_mode = boost::iequals(params.at(0), L"FILE");
	auto path = u8(params.size() > 1 ? params.at(1) : L"");
	auto args = u8(boost::join(params, L" "));

	return spl::make_shared<streaming_consumer>(path, args, compatibility_mode);
}

spl::shared_ptr<core::frame_consumer> create_preconfigured_streaming_consumer(
		const boost::property_tree::wptree& ptree, core::interaction_sink*)
{              	
	return spl::make_shared<streaming_consumer>(
			u8(ptree.get<std::wstring>(L"path")), 
			u8(ptree.get<std::wstring>(L"args", L"")),
			false);
}

}}
