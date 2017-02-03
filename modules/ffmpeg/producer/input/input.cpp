/*
* Copyright 2013 Sveriges Television AB http://casparcg.com/
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

#include "../../StdAfx.h"

#include "input.h"

#include "../util/util.h"
#include "../util/flv.h"
#include "../../ffmpeg_error.h"
#include "../../ffmpeg.h"

#include <core/video_format.h>

#include <common/diagnostics/graph.h>
#include <common/executor.h>
#include <common/except.h>
#include <common/os/general_protection_fault.h>
#include <common/param.h>
#include <common/scope_exit.h>

#include <tbb/concurrent_queue.h>
#include <tbb/atomic.h>
#include <tbb/recursive_mutex.h>

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

static const size_t MAX_BUFFER_COUNT    = 100;
static const size_t MAX_BUFFER_COUNT_RT = 3;
static const size_t MIN_BUFFER_COUNT    = 50;
static const size_t MAX_BUFFER_SIZE     = 64 * 1000000;

namespace caspar { namespace ffmpeg {
struct input::impl : boost::noncopyable
{
	const spl::shared_ptr<diagnostics::graph>					graph_;

	const spl::shared_ptr<AVFormatContext>						format_context_; // Destroy this last
	const int													default_stream_index_	= av_find_default_stream_index(format_context_.get());

	const std::wstring											filename_;
	tbb::atomic<uint32_t>										in_;
	tbb::atomic<uint32_t>										out_;
	const bool													thumbnail_mode_;
	tbb::atomic<bool>											loop_;
	uint32_t													file_frame_number_		= 0;

	tbb::concurrent_bounded_queue<std::shared_ptr<AVPacket>>	buffer_;
	tbb::atomic<size_t>											buffer_size_;

	executor													executor_;

	explicit impl(const spl::shared_ptr<diagnostics::graph> graph, const std::wstring& url_or_file, bool loop, uint32_t in, uint32_t out, bool thumbnail_mode, const ffmpeg_options& vid_params)
		: graph_(graph)
		, format_context_(open_input(url_or_file, vid_params))
		, filename_(url_or_file)
		, thumbnail_mode_(thumbnail_mode)
		, executor_(print())
	{
		if (thumbnail_mode_)
			executor_.invoke([]
			{
				enable_quiet_logging_for_thread();
			});

		in_				= in;
		out_			= out;
		loop_			= loop;
		buffer_size_	= 0;

		if(in_ > 0)
			queued_seek(in_);

		graph_->set_color("seek", diagnostics::color(1.0f, 0.5f, 0.0f));
		graph_->set_color("buffer-count", diagnostics::color(0.7f, 0.4f, 0.4f));
		graph_->set_color("buffer-size", diagnostics::color(1.0f, 1.0f, 0.0f));

		tick();
	}

	bool try_pop(std::shared_ptr<AVPacket>& packet)
	{
		auto result = buffer_.try_pop(packet);

		if(result)
		{
			if(packet)
				buffer_size_ -= packet->size;
			tick();
		}

		graph_->set_value("buffer-size", (static_cast<double>(buffer_size_)+0.001)/MAX_BUFFER_SIZE);
		graph_->set_value("buffer-count", (static_cast<double>(buffer_.size()+0.001)/MAX_BUFFER_COUNT));

		return result;
	}

	std::ptrdiff_t get_max_buffer_count() const
	{
		return thumbnail_mode_ ? 1 : MAX_BUFFER_COUNT;
	}

	std::ptrdiff_t get_min_buffer_count() const
	{
		return thumbnail_mode_ ? 0 : MIN_BUFFER_COUNT;
	}

	std::future<bool> seek(uint32_t target)
	{
		if (!executor_.is_running())
			return make_ready_future(false);

		return executor_.begin_invoke([=]() -> bool
		{
			std::shared_ptr<AVPacket> packet;
			while(buffer_.try_pop(packet) && packet)
				buffer_size_ -= packet->size;

			queued_seek(target);

			tick();

			return true;
		}, task_priority::high_priority);
	}

	std::wstring print() const
	{
		return L"ffmpeg_input[" + filename_ + L")]";
	}

	bool full() const
	{
		return (buffer_size_ > MAX_BUFFER_SIZE || buffer_.size() > get_max_buffer_count()) && buffer_.size() > get_min_buffer_count();
	}

	void tick()
	{
		if(!executor_.is_running())
			return;

		executor_.begin_invoke([this]
		{
			if(full())
				return;

			try
			{
				auto packet = create_packet();

				auto ret = av_read_frame(format_context_.get(), packet.get()); // packet is only valid until next call of av_read_frame. Use av_dup_packet to extend its life.

				if(is_eof(ret))
				{
					file_frame_number_ = 0;

					if(loop_)
					{
						queued_seek(in_);
						graph_->set_tag(diagnostics::tag_severity::INFO, "seek");
						CASPAR_LOG(trace) << print() << " Looping.";
					}
					else
					{
						// Needed by some decoders to decode remaining frames based on last packet.
						auto flush_packet = create_packet();
						flush_packet->data = nullptr;
						flush_packet->size = 0;
						flush_packet->pos = -1;

						buffer_.push(flush_packet);

						executor_.stop();
					}
				}
				else
				{
					THROW_ON_ERROR(ret, "av_read_frame", print());

					if(packet->stream_index == default_stream_index_)
						++file_frame_number_;

					THROW_ON_ERROR2(av_dup_packet(packet.get()), print());

					// Make sure that the packet is correctly deallocated even if size and data is modified during decoding.
					auto size = packet->size;
					auto data = packet->data;

					packet = spl::shared_ptr<AVPacket>(packet.get(), [packet, size, data](AVPacket*)
					{
						packet->size = size;
						packet->data = data;
					});

					buffer_.try_push(packet);
					buffer_size_ += packet->size;

					graph_->set_value("buffer-size", (static_cast<double>(buffer_size_)+0.001)/MAX_BUFFER_SIZE);
					graph_->set_value("buffer-count", (static_cast<double>(buffer_.size()+0.001)/MAX_BUFFER_COUNT));
				}

				tick();
			}
			catch(...)
			{
				if (!thumbnail_mode_)
					CASPAR_LOG_CURRENT_EXCEPTION();
				executor_.stop();
			}
		});
	}

	spl::shared_ptr<AVFormatContext> open_input(const std::wstring& url_or_file, const ffmpeg_options& vid_params)
	{
		AVDictionary* format_options = nullptr;

		CASPAR_SCOPE_EXIT
		{
			if (format_options)
				av_dict_free(&format_options);
		};

		for (auto& option : vid_params)
			av_dict_set(&format_options, option.first.c_str(), option.second.c_str(), 0);

		auto resource_name			= std::wstring();
		auto parts					= caspar::protocol_split(url_or_file);
		auto protocol				= parts.at(0);
		auto path					= parts.at(1);
		AVInputFormat* input_format	= nullptr;

		static const std::set<std::wstring> PROTOCOLS_TREATED_AS_FORMATS = { L"dshow", L"v4l2", L"iec61883" };

		if (protocol.empty())
			resource_name = path;
		else if (PROTOCOLS_TREATED_AS_FORMATS.find(protocol) != PROTOCOLS_TREATED_AS_FORMATS.end())
		{
			input_format = av_find_input_format(u8(protocol).c_str());
			resource_name = path;
		}
		else
			resource_name = protocol + L"://" + path;

		AVFormatContext* weak_context = nullptr;
		THROW_ON_ERROR2(avformat_open_input(&weak_context, u8(resource_name).c_str(), input_format, &format_options), resource_name);

		spl::shared_ptr<AVFormatContext> context(weak_context, [](AVFormatContext* ptr)
		{
			avformat_close_input(&ptr);
		});

		if (format_options)
		{
			std::string unsupported_tokens = "";
			AVDictionaryEntry *t = NULL;
			while ((t = av_dict_get(format_options, "", t, AV_DICT_IGNORE_SUFFIX)) != nullptr)
			{
				if (!unsupported_tokens.empty())
					unsupported_tokens += ", ";
				unsupported_tokens += t->key;
			}
			CASPAR_THROW_EXCEPTION(user_error() << msg_info(unsupported_tokens));
		}

		THROW_ON_ERROR2(avformat_find_stream_info(context.get(), nullptr), resource_name);
		fix_meta_data(*context);
		return context;
	}

	void fix_meta_data(AVFormatContext& context)
	{
		auto video_index = av_find_best_stream(&context, AVMEDIA_TYPE_VIDEO, -1, -1, 0, 0);

		if (video_index > -1)
		{
			auto video_stream = context.streams[video_index];
			auto video_context = context.streams[video_index]->codec;

			if (boost::filesystem::path(context.filename).extension().string() == ".flv")
			{
				try
				{
					auto meta = read_flv_meta_info(context.filename);
					double fps = boost::lexical_cast<double>(meta["framerate"]);
					video_stream->nb_frames = static_cast<int64_t>(boost::lexical_cast<double>(meta["duration"])*fps);
				}
				catch (...) {}
			}
			else
			{
				auto stream_time = video_stream->time_base;
				auto duration = video_stream->duration;
				auto codec_time = video_context->time_base;
				auto ticks = video_context->ticks_per_frame;

				if (video_stream->nb_frames == 0)
					video_stream->nb_frames = (duration*stream_time.num*codec_time.den) / (stream_time.den*codec_time.num*ticks);
			}
		}
	}

	void queued_seek(const uint32_t target)
	{
		if (!thumbnail_mode_)
			CASPAR_LOG(debug) << print() << " Seeking: " << target;

		int flags = AVSEEK_FLAG_FRAME;
		if(target == 0)
		{
			// Fix VP6 seeking
			int vid_stream_index = av_find_best_stream(format_context_.get(), AVMEDIA_TYPE_VIDEO, -1, -1, 0, 0);
			if(vid_stream_index >= 0)
			{
				auto codec_id = format_context_->streams[vid_stream_index]->codec->codec_id;
				if(codec_id == CODEC_ID_VP6A || codec_id == CODEC_ID_VP6F || codec_id == CODEC_ID_VP6)
					flags = AVSEEK_FLAG_BYTE;
			}
		}

		auto stream = format_context_->streams[default_stream_index_];

		auto fps = read_fps(*format_context_, 0.0);

		THROW_ON_ERROR2(avformat_seek_file(
			format_context_.get(),
			default_stream_index_,
			std::numeric_limits<int64_t>::min(),
			static_cast<int64_t>((target / fps * stream->time_base.den) / stream->time_base.num),
			std::numeric_limits<int64_t>::max(),
			0), print());

		file_frame_number_ = target;

		auto flush_packet	= create_packet();
		flush_packet->data	= nullptr;
		flush_packet->size	= 0;
		flush_packet->pos	= target;

		buffer_.push(flush_packet);
	}

	bool is_eof(int ret)
	{
		if(ret == AVERROR(EIO))
			CASPAR_LOG(trace) << print() << " Received EIO, assuming EOF. ";
		if(ret == AVERROR_EOF)
			CASPAR_LOG(trace) << print() << " Received EOF. ";

		return ret == AVERROR_EOF || ret == AVERROR(EIO) || file_frame_number_ >= out_; // av_read_frame doesn't always correctly return AVERROR_EOF;
	}

	int num_audio_streams() const
	{
		return 0; // TODO
	}
};

input::input(const spl::shared_ptr<diagnostics::graph>& graph, const std::wstring& url_or_file, bool loop, uint32_t in, uint32_t out, bool thumbnail_mode, const ffmpeg_options& vid_params)
	: impl_(new impl(graph, url_or_file, loop, in, out, thumbnail_mode, vid_params)){}
bool input::eof() const {return !impl_->executor_.is_running();}
bool input::try_pop(std::shared_ptr<AVPacket>& packet){return impl_->try_pop(packet);}
spl::shared_ptr<AVFormatContext> input::context(){return impl_->format_context_;}
void input::in(uint32_t value){impl_->in_ = value;}
uint32_t input::in() const{return impl_->in_;}
void input::out(uint32_t value){impl_->out_ = value;}
uint32_t input::out() const{return impl_->out_;}
void input::length(uint32_t value){impl_->out_ = impl_->in_ + value;}
uint32_t input::length() const{return impl_->out_ - impl_->in_;}
void input::loop(bool value){impl_->loop_ = value;}
bool input::loop() const{return impl_->loop_;}
int input::num_audio_streams() const { return impl_->num_audio_streams(); }
std::future<bool> input::seek(uint32_t target){return impl_->seek(target);}
}}
