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
* Author: Helge Norberg, helge.norberg@svt.se
* Author: Robert Nagy, ronag89@gmail.com
*/

#include "StdAfx.h"

#include "ffmpeg_pipeline_backend.h"
#include "ffmpeg_pipeline_backend_internal.h"
#include "producer/input/input.h"
#include "producer/video/video_decoder.h"
#include "producer/audio/audio_decoder.h"
#include "producer/filter/audio_filter.h"
#include "producer/filter/filter.h"
#include "producer/util/util.h"
#include "ffmpeg_error.h"
#include "ffmpeg.h"

#include <common/diagnostics/graph.h>
#include <common/os/general_protection_fault.h>
#include <common/enum_class.h>

#include <core/frame/audio_channel_layout.h>
#include <core/frame/frame.h>
#include <core/frame/frame_factory.h>
#include <core/video_format.h>

#include <functional>
#include <limits>
#include <queue>
#include <map>

#include <tbb/atomic.h>
#include <tbb/concurrent_queue.h>
#include <tbb/spin_mutex.h>

#include <boost/thread.hpp>
#include <boost/optional.hpp>
#include <boost/exception_ptr.hpp>

namespace caspar { namespace ffmpeg {

std::string to_string(const boost::rational<int>& framerate)
{
	return boost::lexical_cast<std::string>(framerate.numerator())
		+ "/" + boost::lexical_cast<std::string>(framerate.denominator()) + " (" + boost::lexical_cast<std::string>(static_cast<double>(framerate.numerator()) / static_cast<double>(framerate.denominator())) + ") fps";
}

std::vector<int> find_audio_cadence(const boost::rational<int>& framerate)
{
	static std::map<boost::rational<int>, std::vector<int>> CADENCES_BY_FRAMERATE = []
	{
		std::map<boost::rational<int>, std::vector<int>> result;

		for (core::video_format format : enum_constants<core::video_format>())
		{
			core::video_format_desc desc(format);
			boost::rational<int> format_rate(desc.time_scale, desc.duration);

			result.insert(std::make_pair(format_rate, desc.audio_cadence));
		}

		return result;
	}();

	auto exact_match = CADENCES_BY_FRAMERATE.find(framerate);

	if (exact_match != CADENCES_BY_FRAMERATE.end())
		return exact_match->second;

	boost::rational<int> closest_framerate_diff	= std::numeric_limits<int>::max();
	boost::rational<int> closest_framerate		= 0;

	for (auto format_framerate : CADENCES_BY_FRAMERATE | boost::adaptors::map_keys)
	{
		auto diff = boost::abs(framerate - format_framerate);

		if (diff < closest_framerate_diff)
		{
			closest_framerate_diff	= diff;
			closest_framerate		= format_framerate;
		}
	}

	if (is_logging_quiet_for_thread())
		CASPAR_LOG(debug) << "No exact audio cadence match found for framerate " << to_string(framerate)
			<< "\nClosest match is " << to_string(closest_framerate)
			<< "\nwhich is a " << to_string(closest_framerate_diff) << " difference.";
	else
		CASPAR_LOG(warning) << "No exact audio cadence match found for framerate " << to_string(framerate)
			<< "\nClosest match is " << to_string(closest_framerate)
			<< "\nwhich is a " << to_string(closest_framerate_diff) << " difference.";

	return CADENCES_BY_FRAMERATE[closest_framerate];
}

struct source
{
	virtual ~source() { }

	virtual std::wstring							print() const											= 0;
	virtual void									start()													{ CASPAR_THROW_EXCEPTION(not_implemented() << msg_info(print())); }
	virtual void									graph(spl::shared_ptr<caspar::diagnostics::graph> g)	{ }
	virtual void									stop()													{ }
	virtual void									start_frame(std::uint32_t frame)						{ CASPAR_THROW_EXCEPTION(invalid_operation() << msg_info(print() + L" not seekable.")); }
	virtual std::uint32_t							start_frame() const										{ CASPAR_THROW_EXCEPTION(invalid_operation() << msg_info(print() + L" not seekable.")); }
	virtual void									loop(bool value)										{ CASPAR_THROW_EXCEPTION(invalid_operation() << msg_info(print() + L" not seekable.")); }
	virtual bool									loop() const											{ CASPAR_THROW_EXCEPTION(invalid_operation() << msg_info(print() + L" not seekable.")); }
	virtual void									length(std::uint32_t frames)							{ CASPAR_THROW_EXCEPTION(invalid_operation() << msg_info(print() + L" not seekable.")); }
	virtual std::uint32_t							length() const											{ CASPAR_THROW_EXCEPTION(invalid_operation() << msg_info(print() + L" not seekable.")); }
	virtual std::string								filename() const										{ CASPAR_THROW_EXCEPTION(invalid_operation() << msg_info(print())); }
	virtual void									seek(std::uint32_t frame)								{ CASPAR_THROW_EXCEPTION(invalid_operation() << msg_info(print() + L" not seekable.")); }
	virtual bool									has_audio() const										{ CASPAR_THROW_EXCEPTION(not_implemented() << msg_info(print())); }
	virtual int										samplerate() const										{ CASPAR_THROW_EXCEPTION(not_implemented() << msg_info(print())); }
	virtual bool									has_video() const										{ CASPAR_THROW_EXCEPTION(not_implemented() << msg_info(print())); }
	virtual bool									eof() const												{ CASPAR_THROW_EXCEPTION(not_implemented() << msg_info(print())); }
	virtual boost::rational<int>					framerate() const										{ CASPAR_THROW_EXCEPTION(not_implemented() << msg_info(print())); }
	virtual std::uint32_t							frame_number() const									{ CASPAR_THROW_EXCEPTION(not_implemented() << msg_info(print())); }
	virtual std::vector<std::shared_ptr<AVFrame>>	get_input_frames_for_streams(AVMediaType type)			{ CASPAR_THROW_EXCEPTION(not_implemented() << msg_info(print())); }
};

struct no_source_selected : public source
{
	std::wstring print() const override
	{
		return L"[no_source_selected]";
	}
};

class file_source : public source
{
	std::wstring								filename_;
	spl::shared_ptr<diagnostics::graph>			graph_;
	std::uint32_t								start_frame_	= 0;
	std::uint32_t								length_			= std::numeric_limits<std::uint32_t>::max();
	bool										loop_			= false;
	mutable boost::mutex						pointer_mutex_;
	std::shared_ptr<input>						input_;
	std::vector<spl::shared_ptr<audio_decoder>>	audio_decoders_;
	std::shared_ptr<video_decoder>				video_decoder_;
	bool										started_		= false;
public:
	file_source(std::string filename)
		: filename_(u16(filename))
	{
	}

	std::wstring print() const override
	{
		return L"[file_source " + filename_ + L"]";
	}

	void graph(spl::shared_ptr<caspar::diagnostics::graph> g) override
	{
		graph_ = std::move(g);
	}

	void start() override
	{
		boost::lock_guard<boost::mutex> lock(pointer_mutex_);
		bool thumbnail_mode = is_logging_quiet_for_thread();
		input_.reset(new input(graph_, filename_, loop_, start_frame_, length_, thumbnail_mode));

		for (int i = 0; i < input_->num_audio_streams(); ++i)
		{
			try
			{
				audio_decoders_.push_back(spl::make_shared<audio_decoder>(*input_, core::video_format::invalid, i));
			}
			catch (...)
			{
				if (is_logging_quiet_for_thread())
				{
					CASPAR_LOG_CURRENT_EXCEPTION_AT_LEVEL(debug);
					CASPAR_LOG(info) << print() << " Failed to open audio-stream. Turn on log level debug to see more information.";
				}
				else
				{
					CASPAR_LOG_CURRENT_EXCEPTION();
					CASPAR_LOG(warning) << print() << " Failed to open audio-stream.";
				}
			}
		}

		if (audio_decoders_.empty())
			CASPAR_LOG(debug) << print() << " No audio-stream found. Running without audio.";

		try
		{
			video_decoder_.reset(new video_decoder(*input_, false));
		}
		catch (averror_stream_not_found&)
		{
			CASPAR_LOG(debug) << print() << " No video-stream found. Running without video.";
		}
		catch (...)
		{
			if (is_logging_quiet_for_thread())
			{
				CASPAR_LOG_CURRENT_EXCEPTION_AT_LEVEL(debug);
				CASPAR_LOG(info) << print() << " Failed to open video-stream. Running without audio. Turn on log level debug to see more information.";
			}
			else
			{
				CASPAR_LOG_CURRENT_EXCEPTION();
				CASPAR_LOG(warning) << print() << " Failed to open video-stream. Running without audio.";
			}
		}

		started_ = true;
	}

	void stop() override
	{
		started_ = false;
	}

	void start_frame(std::uint32_t frame) override 
	{
		start_frame_ = frame;

		auto i = get_input();
		if (i)
			i->start(frame);
	}

	std::uint32_t start_frame() const override
	{
		return start_frame_;
	}

	void loop(bool value) override
	{
		loop_ = value;

		auto i = get_input();
		if (i)
			i->loop(value);
	}

	bool loop() const override
	{
		return loop_;
	}

	void length(std::uint32_t frames) override
	{
		length_ = frames;

		auto i = get_input();
		if (i)
			i->length(frames);
	}

	std::uint32_t length() const override
	{
		auto v = get_video_decoder();

		if (v)
			return v->nb_frames();

		auto a = get_audio_decoders();

		if (!a.empty())
			return a.at(0)->nb_frames(); // Should be ok.

		return length_;
	}

	std::string filename() const override
	{
		return u8(filename_);
	}

	void seek(std::uint32_t frame) override
	{
		expect_started();
		get_input()->seek(frame);
	}

	bool eof() const override
	{
		auto i = get_input();
		return !i || i->eof();
	}

	bool has_audio() const override
	{
		return !get_audio_decoders().empty();
	}

	int samplerate() const override
	{
		if (get_audio_decoders().empty())
			return -1;

		return 48000;
	}

	bool has_video() const override
	{
		return static_cast<bool>(get_video_decoder());
	}

	boost::rational<int> framerate() const override
	{
		auto decoder = get_video_decoder();

		if (!decoder)
			return -1;

		return decoder->framerate();
	}

	std::uint32_t frame_number() const override
	{
		auto decoder = get_video_decoder();

		if (!decoder)
			return 0;

		return decoder->file_frame_number();
	}

	std::vector<std::shared_ptr<AVFrame>> get_input_frames_for_streams(AVMediaType type) override
	{
		auto a_decoders	= get_audio_decoders();
		auto v_decoder	= get_video_decoder();
		expect_started();

		if (type == AVMediaType::AVMEDIA_TYPE_AUDIO && !a_decoders.empty())
		{
			std::vector<std::shared_ptr<AVFrame>> frames;

			for (auto& a_decoder : a_decoders)
			{
				std::shared_ptr<AVFrame> frame;

				for (int i = 0; i < 64; ++i)
				{
					frame = (*a_decoder)();

					if (frame && frame->data[0])
						break;
					else
						frame.reset();
				}

				frames.push_back(std::move(frame));
			}

			return frames;
		}
		else if (type == AVMediaType::AVMEDIA_TYPE_VIDEO && v_decoder)
		{
			std::shared_ptr<AVFrame> frame;

			for (int i = 0; i < 128; ++i)
			{
				frame = (*v_decoder)();

				if (frame && frame->data[0])
					return { frame };
			}
		}
		else
			CASPAR_THROW_EXCEPTION(caspar_exception() << msg_info(
				print() + L" Unhandled media type " + boost::lexical_cast<std::wstring>(type)));

		return { };
	}
private:
	void expect_started() const
	{
		if (!started_)
			CASPAR_THROW_EXCEPTION(invalid_operation() << msg_info(print() + L" Not started."));
	}

	std::shared_ptr<input> get_input() const
	{
		boost::lock_guard<boost::mutex> lock(pointer_mutex_);
		return input_;
	}

	std::vector<spl::shared_ptr<audio_decoder>> get_audio_decoders() const
	{
		boost::lock_guard<boost::mutex> lock(pointer_mutex_);
		return audio_decoders_;
	}

	std::shared_ptr<video_decoder> get_video_decoder() const
	{
		boost::lock_guard<boost::mutex> lock(pointer_mutex_);
		return video_decoder_;
	}
};

class memory_source : public source
{
	int															samplerate_		= -1;
	int															num_channels_	= -1;
	int															width_			= -1;
	int															height_			= -1;
	boost::rational<int>										framerate_		= -1;

	tbb::atomic<bool>											running_;
	tbb::concurrent_bounded_queue<caspar::array<const int32_t>>	audio_frames_;
	tbb::concurrent_bounded_queue<caspar::array<const uint8_t>>	video_frames_;
	int64_t														audio_pts_		= 0;
	int64_t														video_pts_		= 0;
public:
	memory_source()
	{
		running_ = false;
		video_frames_.set_capacity(1);
		audio_frames_.set_capacity(1);
	}

	~memory_source()
	{
		stop();
	}

	void graph(spl::shared_ptr<caspar::diagnostics::graph> g) override
	{
	}

	std::wstring print() const override
	{
		return L"[memory_source]";
	}

	void enable_audio(int samplerate, int num_channels)
	{
		samplerate_ = samplerate;
		num_channels_ = num_channels;
	}

	void enable_video(int width, int height, boost::rational<int> framerate)
	{
		width_ = width;
		height_ = height;
	}

	void start() override
	{
		running_ = true;
	}

	void stop() override
	{
		running_ = false;
		video_frames_.try_push(caspar::array<const uint8_t>());
		audio_frames_.try_push(caspar::array<const int32_t>());
	}

	bool has_audio() const override
	{
		return samplerate_ != -1;
	}

	int samplerate() const override
	{
		return samplerate_;
	}

	bool has_video() const override
	{
		return width_ != -1;
	}

	bool eof() const override
	{
		return !running_;
	}

	boost::rational<int> framerate() const override
	{
		return framerate_;
	}
	
	bool try_push_audio(caspar::array<const std::int32_t> data)
	{
		if (!has_audio())
			CASPAR_THROW_EXCEPTION(invalid_operation() << msg_info(print() + L" audio not enabled."));

		if (data.empty() || data.size() % num_channels_ != 0)
			CASPAR_THROW_EXCEPTION(invalid_argument() << msg_info(print() + L" audio with incorrect number of channels submitted."));

		return audio_frames_.try_push(std::move(data));
	}

	bool try_push_video(caspar::array<const std::uint8_t> data)
	{
		if (!has_video())
			CASPAR_THROW_EXCEPTION(invalid_operation() << msg_info(print() + L" video not enabled."));

		if (data.size() != width_ * height_ * 4)
			CASPAR_THROW_EXCEPTION(invalid_argument() << msg_info(print() + L" video with incorrect size submitted."));

		return video_frames_.try_push(std::move(data));
	}

	std::vector<std::shared_ptr<AVFrame>> get_input_frames_for_streams(AVMediaType type) override
	{
		if (!running_)
			CASPAR_THROW_EXCEPTION(invalid_operation() << msg_info(print() + L" not running."));

		if (type == AVMediaType::AVMEDIA_TYPE_AUDIO && has_audio())
		{
			caspar::array<const std::int32_t> samples;
			audio_frames_.pop(samples);

			if (samples.empty())
				return { };
			
			spl::shared_ptr<AVFrame> av_frame(av_frame_alloc(), [samples](AVFrame* p) { av_frame_free(&p); });

			av_frame->channels			= num_channels_;
			av_frame->channel_layout	= av_get_default_channel_layout(num_channels_);
			av_frame->sample_rate		= samplerate_;
			av_frame->nb_samples		= static_cast<int>(samples.size()) / num_channels_;
			av_frame->format			= AV_SAMPLE_FMT_S32;
			av_frame->pts				= audio_pts_;

			audio_pts_ += av_frame->nb_samples;

			FF(av_samples_fill_arrays(
					av_frame->extended_data,
					av_frame->linesize,
					reinterpret_cast<const std::uint8_t*>(&*samples.begin()),
					av_frame->channels,
					av_frame->nb_samples,
					static_cast<AVSampleFormat>(av_frame->format),
					16));

			return { av_frame };
		}
		else if (type == AVMediaType::AVMEDIA_TYPE_VIDEO && has_video())
		{
			caspar::array<const std::uint8_t> data;
			video_frames_.pop(data);

			if (data.empty())
				return {};

			spl::shared_ptr<AVFrame> av_frame(av_frame_alloc(), [data](AVFrame* p) { av_frame_free(&p); });
			avcodec_get_frame_defaults(av_frame.get());		
			
			const auto sample_aspect_ratio = boost::rational<int>(width_, height_);

			av_frame->format				  = AV_PIX_FMT_BGRA;
			av_frame->width					  = width_;
			av_frame->height				  = height_;
			av_frame->sample_aspect_ratio.num = sample_aspect_ratio.numerator();
			av_frame->sample_aspect_ratio.den = sample_aspect_ratio.denominator();
			av_frame->pts					  = video_pts_;

			video_pts_ += 1;

			FF(av_image_fill_arrays(
					av_frame->data,
					av_frame->linesize,
					data.begin(),
					static_cast<AVPixelFormat>(av_frame->format),
					width_,
					height_,
					1));

			return { av_frame };
		}
		else
			CASPAR_THROW_EXCEPTION(caspar_exception() << msg_info(
				print() + L" Unhandled media type " + boost::lexical_cast<std::wstring>(type)));
	}
};

struct sink
{
	virtual ~sink() { }

	virtual std::wstring					print() const																	= 0;
	virtual void							graph(spl::shared_ptr<caspar::diagnostics::graph> g)							{ }
	virtual void							acodec(std::string codec)														{ CASPAR_THROW_EXCEPTION(invalid_operation() << msg_info(print() + L" not an encoder.")); }
	virtual void							vcodec(std::string codec)														{ CASPAR_THROW_EXCEPTION(invalid_operation() << msg_info(print() + L" not an encoder.")); }
	virtual void							format(std::string fmt)															{ CASPAR_THROW_EXCEPTION(invalid_operation() << msg_info(print() + L" not an encoder.")); }
	virtual void							framerate(boost::rational<int> framerate)										{ CASPAR_THROW_EXCEPTION(invalid_operation() << msg_info(print() + L" not an encoder.")); }
	virtual void							start(bool has_audio, bool has_video)											{ CASPAR_THROW_EXCEPTION(not_implemented() << msg_info(print())); }
	virtual void							stop()																			{ }
	virtual std::vector<AVSampleFormat>		supported_sample_formats() const												{ CASPAR_THROW_EXCEPTION(not_implemented() << msg_info(print())); }
	virtual std::vector<int>				supported_samplerates() const													{ CASPAR_THROW_EXCEPTION(not_implemented() << msg_info(print())); }
	virtual std::vector<AVPixelFormat>		supported_pixel_formats() const													{ CASPAR_THROW_EXCEPTION(not_implemented() << msg_info(print())); }
	virtual int								wanted_num_audio_streams() const												{ CASPAR_THROW_EXCEPTION(not_implemented() << msg_info(print())); }
	virtual boost::optional<int>			wanted_num_channels_per_stream() const										{ CASPAR_THROW_EXCEPTION(not_implemented() << msg_info(print())); }
	virtual boost::optional<AVMediaType>	try_push(AVMediaType type, int stream_index, spl::shared_ptr<AVFrame> frame)	{ CASPAR_THROW_EXCEPTION(not_implemented() << msg_info(print())); }
	virtual void							eof()																			{ CASPAR_THROW_EXCEPTION(not_implemented() << msg_info(print())); }
};

struct no_sink_selected : public sink
{
	std::wstring print() const override
	{
		return L"[no_sink_selected]";
	}
};

class file_sink : public sink
{
	std::wstring						filename_;
	spl::shared_ptr<diagnostics::graph>	graph_;
public:
	file_sink(std::string filename)
		: filename_(u16(std::move(filename)))
	{
	}

	std::wstring print() const override
	{
		return L"[file_sink " + filename_ + L"]";
	}

	void graph(spl::shared_ptr<caspar::diagnostics::graph> g) override
	{
		graph_ = std::move(g);
	}
};

class memory_sink : public sink
{
	spl::shared_ptr<core::frame_factory>			factory_;

	bool											has_audio_			= false;
	bool											has_video_			= false;
	std::vector<int>								audio_cadence_;
	core::audio_channel_layout						channel_layout_		= core::audio_channel_layout::invalid();
	core::mutable_audio_buffer						audio_samples_;

	std::queue<std::shared_ptr<AVFrame>>			video_frames_;

	tbb::concurrent_bounded_queue<core::draw_frame>	output_frames_;
	tbb::atomic<bool>								running_;
public:
	memory_sink(spl::shared_ptr<core::frame_factory> factory, core::video_format_desc format)
		: factory_(std::move(factory))
		, audio_cadence_(format.audio_cadence)
	{
		output_frames_.set_capacity(2);
		running_ = false;
		// Note: Uses 1 step rotated cadence for 1001 modes (1602, 1602, 1601, 1602, 1601)
		// This cadence fills the audio mixer most optimally.
		boost::range::rotate(audio_cadence_, std::end(audio_cadence_) - 1);
	}

	~memory_sink()
	{
		stop();
	}

	std::wstring print() const override
	{
		return L"[memory_sink]";
	}

	void graph(spl::shared_ptr<caspar::diagnostics::graph> g) override
	{
	}

	void framerate(boost::rational<int> framerate) override
	{
		audio_cadence_ = find_audio_cadence(framerate);
		// Note: Uses 1 step rotated cadence for 1001 modes (1602, 1602, 1601, 1602, 1601)
		// This cadence fills the audio mixer most optimally.
		boost::range::rotate(audio_cadence_, std::end(audio_cadence_) - 1);
	}

	void start(bool has_audio, bool has_video) override
	{
		has_audio_	= has_audio;
		has_video_	= has_video;
		running_	= true;
	}

	void stop() override
	{
		running_ = false;
		output_frames_.set_capacity(4);
	}

	std::vector<AVSampleFormat> supported_sample_formats() const override
	{
		return { AVSampleFormat::AV_SAMPLE_FMT_S32 };
	}

	std::vector<int> supported_samplerates() const override {
		return { 48000 };
	}

	std::vector<AVPixelFormat> supported_pixel_formats() const override
	{
		return {
			AVPixelFormat::AV_PIX_FMT_YUVA420P,
			AVPixelFormat::AV_PIX_FMT_YUV444P,
			AVPixelFormat::AV_PIX_FMT_YUV422P,
			AVPixelFormat::AV_PIX_FMT_YUV420P,
			AVPixelFormat::AV_PIX_FMT_YUV411P,
			AVPixelFormat::AV_PIX_FMT_BGRA,
			AVPixelFormat::AV_PIX_FMT_ARGB,
			AVPixelFormat::AV_PIX_FMT_RGBA,
			AVPixelFormat::AV_PIX_FMT_ABGR,
			AVPixelFormat::AV_PIX_FMT_GRAY8
		};
	}

	int wanted_num_audio_streams() const override
	{
		return 1;
	}

	boost::optional<int> wanted_num_channels_per_stream() const
	{
		return boost::none;
	}

	boost::optional<AVMediaType> try_push(AVMediaType type, int stream_index, spl::shared_ptr<AVFrame> av_frame) override
	{
		if (!has_audio_ && !has_video_)
			CASPAR_THROW_EXCEPTION(invalid_operation());

		if (type == AVMediaType::AVMEDIA_TYPE_AUDIO && av_frame->data[0])
		{
			if (channel_layout_ == core::audio_channel_layout::invalid()) // First audio
			{
				channel_layout_ = get_audio_channel_layout(av_frame->channels, av_frame->channel_layout, L"");

				// Insert silence samples so that the audio mixer is guaranteed to be filled.
				auto min_num_samples_per_frame	= *boost::min_element(audio_cadence_);
				auto max_num_samples_per_frame	= *boost::max_element(audio_cadence_);
				auto cadence_safety_samples		= max_num_samples_per_frame - min_num_samples_per_frame;
				audio_samples_.resize(channel_layout_.num_channels * cadence_safety_samples, 0);
			}

			auto ptr = reinterpret_cast<int32_t*>(av_frame->data[0]);

			audio_samples_.insert(audio_samples_.end(), ptr, ptr + av_frame->linesize[0] / sizeof(int32_t));
		}
		else if (type == AVMediaType::AVMEDIA_TYPE_VIDEO)
		{
			video_frames_.push(std::move(av_frame));
		}

		while (true)
		{
			bool enough_audio =
				!has_audio_ ||
				(channel_layout_ != core::audio_channel_layout::invalid() && audio_samples_.size() >= audio_cadence_.front() * channel_layout_.num_channels);
			bool enough_video =
				!has_video_ ||
				!video_frames_.empty();

			if (!enough_audio)
				return AVMediaType::AVMEDIA_TYPE_AUDIO;

			if (!enough_video)
				return AVMediaType::AVMEDIA_TYPE_VIDEO;

			core::mutable_audio_buffer audio_data;

			if (has_audio_)
			{
				auto begin = audio_samples_.begin();
				auto end = begin + audio_cadence_.front() * channel_layout_.num_channels;

				audio_data.insert(audio_data.begin(), begin, end);
				audio_samples_.erase(begin, end);
				boost::range::rotate(audio_cadence_, std::begin(audio_cadence_) + 1);
			}

			if (!has_video_) // Audio only
			{
				core::mutable_frame audio_only_frame(
						{ },
						std::move(audio_data),
						this,
						core::pixel_format_desc(core::pixel_format::invalid),
						channel_layout_);

				output_frames_.push(core::draw_frame(std::move(audio_only_frame)));

				return AVMediaType::AVMEDIA_TYPE_AUDIO;
			}

			auto output_frame = make_frame(this, spl::make_shared_ptr(video_frames_.front()), *factory_, channel_layout_);
			video_frames_.pop();
			output_frame.audio_data() = std::move(audio_data);

			output_frames_.push(core::draw_frame(std::move(output_frame)));
		}
	}

	void eof() override
	{
		// Drain rest, regardless of it being enough or not.
		while (!video_frames_.empty() || !audio_samples_.empty())
		{
			core::mutable_audio_buffer audio_data;

			audio_data.swap(audio_samples_);

			if (!video_frames_.empty())
			{
				auto output_frame = make_frame(this, spl::make_shared_ptr(video_frames_.front()), *factory_, channel_layout_);
				video_frames_.pop();
				output_frame.audio_data() = std::move(audio_data);

				output_frames_.push(core::draw_frame(std::move(output_frame)));
			}
			else
			{
				core::mutable_frame audio_only_frame(
						{},
						std::move(audio_data),
						this,
						core::pixel_format_desc(core::pixel_format::invalid),
						channel_layout_);

				output_frames_.push(core::draw_frame(std::move(audio_only_frame)));
				output_frames_.push(core::draw_frame::empty());
			}
		}
	}

	core::draw_frame try_pop_frame()
	{
		core::draw_frame frame = core::draw_frame::late();

		if (!output_frames_.try_pop(frame) && !running_)
			return core::draw_frame::empty();

		return frame;
	}
};

struct audio_stream_info
{
	int				num_channels	= 0;
	AVSampleFormat	sampleformat	= AVSampleFormat::AV_SAMPLE_FMT_NONE;
	uint64_t		channel_layout	= 0;
};

struct video_stream_info
{
	int					width		= 0;
	int					height		= 0;
	AVPixelFormat		pixelformat	= AVPixelFormat::AV_PIX_FMT_NONE;
	core::field_mode	fieldmode	= core::field_mode::progressive;
};

class ffmpeg_pipeline_backend_internal : public ffmpeg_pipeline_backend
{
	spl::shared_ptr<diagnostics::graph>								graph_;

	spl::unique_ptr<source>											source_					= spl::make_unique<no_source_selected>();
	std::function<bool (caspar::array<const std::int32_t> data)>	try_push_audio_;
	std::function<bool (caspar::array<const std::uint8_t> data)>	try_push_video_;

	std::vector<audio_stream_info>									source_audio_streams_;
	video_stream_info												source_video_stream_;

	std::string														afilter_;
	std::unique_ptr<audio_filter>									audio_filter_;
	std::string														vfilter_;
	std::unique_ptr<filter>											video_filter_;

	spl::unique_ptr<sink>											sink_					= spl::make_unique<no_sink_selected>();
	std::function<core::draw_frame ()>								try_pop_frame_;

	tbb::atomic<bool>												started_;
	tbb::spin_mutex													exception_mutex_;
	boost::exception_ptr											exception_;
	boost::thread													thread_;
public:
	ffmpeg_pipeline_backend_internal()
	{
		started_ = false;
		diagnostics::register_graph(graph_);
	}

	~ffmpeg_pipeline_backend_internal()
	{
		stop();
	}

	void throw_if_error()
	{
		boost::lock_guard<tbb::spin_mutex> lock(exception_mutex_);

		if (exception_ != nullptr)
			boost::rethrow_exception(exception_);
	}

	void graph(spl::shared_ptr<caspar::diagnostics::graph> g) override
	{
		graph_ = std::move(g);
		source_->graph(graph_);
		sink_->graph(graph_);
	}

	// Source setup

	void from_file(std::string filename) override
	{
		source_			= spl::make_unique<file_source>(std::move(filename));
		try_push_audio_	= std::function<bool (caspar::array<const std::int32_t>)>();
		try_push_video_	= std::function<bool (caspar::array<const std::uint8_t>)>();
		source_->graph(graph_);
	}

	void from_memory_only_audio(int num_channels, int samplerate) override
	{
		auto source		= spl::make_unique<memory_source>();
		auto source_ptr	= source.get();
		try_push_audio_	= [this, source_ptr](caspar::array<const std::int32_t> data) { return source_ptr->try_push_audio(std::move(data)); };
		source->enable_audio(samplerate, num_channels);

		source_ = std::move(source);
		source_->graph(graph_);
	}

	void from_memory_only_video(int width, int height, boost::rational<int> framerate) override
	{
		auto source		= spl::make_unique<memory_source>();
		auto source_ptr	= source.get();
		try_push_video_	= [this, source_ptr](caspar::array<const std::uint8_t> data) { return source_ptr->try_push_video(std::move(data)); };
		source->enable_video(width, height, std::move(framerate));

		source_ = std::move(source);
		source_->graph(graph_);
	}

	void from_memory(int num_channels, int samplerate, int width, int height, boost::rational<int> framerate) override
	{
		auto source		= spl::make_unique<memory_source>();
		auto source_ptr	= source.get();
		try_push_audio_	= [this, source_ptr](caspar::array<const std::int32_t> data) { return source_ptr->try_push_audio(std::move(data)); };
		try_push_video_	= [this, source_ptr](caspar::array<const std::uint8_t> data) { return source_ptr->try_push_video(std::move(data)); };
		source->enable_audio(samplerate, num_channels);
		source->enable_video(width, height, std::move(framerate));

		source_ = std::move(source);
		source_->graph(graph_);
	}

	void			start_frame(std::uint32_t frame) override	{ source_->start_frame(frame);		}
	std::uint32_t	start_frame() const override				{ return source_->start_frame();	}
	void			length(std::uint32_t frames) override		{ source_->length(frames);			}
	std::uint32_t	length() const override						{ return source_->length();			}
	void			seek(std::uint32_t frame) override			{ source_->seek(frame);				}
	void			loop(bool value) override					{ source_->loop(value);				}
	bool			loop() const override						{ return source_->loop();			}
	std::string		source_filename() const override			{ return source_->filename();		}

	// Filter setup

	void vfilter(std::string filter) override
	{
		vfilter_ = std::move(filter);
	}

	void afilter(std::string filter) override
	{
		afilter_ = std::move(filter);
	}

	int width() const override
	{
		return source_video_stream_.width;
	}

	int height() const override
	{
		return source_video_stream_.height;
	}

	boost::rational<int> framerate() const override
	{
		bool double_rate = filter::is_double_rate(u16(vfilter_));

		return double_rate ? source_->framerate() * 2 : source_->framerate();
	}

	bool progressive() const override
	{
		return true;//TODO
	}

	// Sink setup

	void to_memory(spl::shared_ptr<core::frame_factory> factory, core::video_format_desc format) override
	{
		auto sink		= spl::make_unique<memory_sink>(std::move(factory), std::move(format));
		auto sink_ptr	= sink.get();
		try_pop_frame_	= [sink_ptr] { return sink_ptr->try_pop_frame(); };

		sink_ = std::move(sink);
		sink_->graph(graph_);
	}

	void to_file(std::string filename) override
	{
		sink_			= spl::make_unique<file_sink>(std::move(filename));
		try_pop_frame_	= std::function<core::draw_frame ()>();
		sink_->graph(graph_);
	}

	void acodec(std::string codec) override	{ sink_->acodec(std::move(codec)); }
	void vcodec(std::string codec) override	{ sink_->vcodec(std::move(codec)); }
	void format(std::string fmt) override	{ sink_->format(std::move(fmt)); }

	// Runtime control

	void start() override
	{
		source_->start();
		sink_->start(source_->has_audio(), source_->has_video());
		started_ = true;
		bool quiet = is_logging_quiet_for_thread();

		thread_ = boost::thread([=] { run(quiet); });
	}

	bool try_push_audio(caspar::array<const std::int32_t> data) override
	{
		throw_if_error();

		if (try_push_audio_)
			return try_push_audio_(std::move(data));
		else
			return false;
	}

	bool try_push_video(caspar::array<const std::uint8_t> data) override
	{
		throw_if_error();

		if (try_push_video_)
			return try_push_video_(std::move(data));
		else
			return false;
	}

	core::draw_frame try_pop_frame() override
	{
		throw_if_error();

		if (!try_pop_frame_)
			CASPAR_THROW_EXCEPTION(invalid_operation());

		return try_pop_frame_();
	}

	std::uint32_t last_frame() const override
	{
		return source_->frame_number();
	}

	bool started() const override
	{
		return started_;
	}

	void stop() override
	{
		started_ = false;

		sink_->stop();
		source_->stop();

		if (thread_.joinable())
			thread_.join();
	}

private:
	void run(bool quiet)
	{
		ensure_gpf_handler_installed_for_thread(u8(L"ffmpeg-pipeline: " + source_->print() + L" -> " + sink_->print()).c_str());
		auto quiet_logging = temporary_enable_quiet_logging_for_thread(quiet);

		try
		{
			boost::optional<AVMediaType> result = source_->has_audio() ? AVMediaType::AVMEDIA_TYPE_AUDIO : AVMediaType::AVMEDIA_TYPE_VIDEO;

			while (started_ && (source_->has_audio() || source_->has_video()))
			{
				auto needed						= *result;
				auto input_frames_for_streams	= source_->get_input_frames_for_streams(needed);

				if (!input_frames_for_streams.empty() && input_frames_for_streams.at(0))
				{
					for (int input_stream_index = 0; input_stream_index < input_frames_for_streams.size(); ++input_stream_index)
					{
						if (needed == AVMediaType::AVMEDIA_TYPE_AUDIO)
						{
							initialize_audio_filter_if_needed(input_frames_for_streams);
							audio_filter_->push(input_stream_index, std::move(input_frames_for_streams.at(input_stream_index)));

							for (int output_stream_index = 0; output_stream_index < sink_->wanted_num_audio_streams(); ++output_stream_index)
								for (auto filtered_frame : audio_filter_->poll_all(output_stream_index))
									result = sink_->try_push(AVMediaType::AVMEDIA_TYPE_AUDIO, output_stream_index, std::move(filtered_frame));
						}
						else if (needed == AVMediaType::AVMEDIA_TYPE_VIDEO)
						{
							initialize_video_filter_if_needed(*input_frames_for_streams.at(input_stream_index));
							video_filter_->push(std::move(input_frames_for_streams.at(input_stream_index)));

							for (auto filtered_frame : video_filter_->poll_all())
								result = sink_->try_push(AVMediaType::AVMEDIA_TYPE_VIDEO, 0, std::move(filtered_frame));
						}
						else
							CASPAR_THROW_EXCEPTION(not_supported());
					}
				}
				else if (source_->eof())
				{
					started_ = false;
					sink_->eof();
					break;
				}
				else
					result = boost::none;

				if (!result)
				{
					graph_->set_tag(caspar::diagnostics::tag_severity::WARNING, "dropped-frame");
					result = needed; // Repeat same media type
				}
			}
		}
		catch (...)
		{
			if (is_logging_quiet_for_thread())
			{
				CASPAR_LOG_CURRENT_EXCEPTION_AT_LEVEL(debug);
			}
			else
			{
				CASPAR_LOG_CURRENT_EXCEPTION();
			}

			boost::lock_guard<tbb::spin_mutex> lock(exception_mutex_);
			exception_ = boost::current_exception();
		}

		video_filter_.reset();
		audio_filter_.reset();
		source_->stop();
		sink_->stop();
		started_ = false;
	}

	template<typename T>
	void set_if_changed(bool& changed, T& old_value, T new_value)
	{
		if (old_value != new_value)
		{
			changed = true;
			old_value = new_value;
		}
	}

	void initialize_audio_filter_if_needed(const std::vector<std::shared_ptr<AVFrame>>& av_frames_per_stream)
	{
		bool changed = av_frames_per_stream.size() != source_audio_streams_.size();
		source_audio_streams_.resize(av_frames_per_stream.size());

		for (int i = 0; i < av_frames_per_stream.size(); ++i)
		{
			auto& av_frame	= *av_frames_per_stream.at(i);
			auto& stream	= source_audio_streams_.at(i);

			auto channel_layout = av_frame.channel_layout == 0
					? av_get_default_channel_layout(av_frame.channels)
					: av_frame.channel_layout;

			set_if_changed(changed, stream.sampleformat, static_cast<AVSampleFormat>(av_frame.format));
			set_if_changed(changed, stream.num_channels, av_frame.channels);
			set_if_changed(changed, stream.channel_layout, channel_layout);
		}

		if (changed)
			initialize_audio_filter();
	}

	void initialize_audio_filter()
	{
		std::vector<audio_input_pad> input_pads;
		std::vector<audio_output_pad> output_pads;

		for (auto& source_audio_stream : source_audio_streams_)
		{
			input_pads.emplace_back(
					boost::rational<int>(1, source_->samplerate()),
					source_->samplerate(),
					source_audio_stream.sampleformat,
					source_audio_stream.channel_layout);
		}

		auto total_num_channels = cpplinq::from(source_audio_streams_)
				.select([](const audio_stream_info& info) { return info.num_channels; })
				.aggregate(0, std::plus<int>());

		if (total_num_channels > 1 && sink_->wanted_num_audio_streams() > 1)
			CASPAR_THROW_EXCEPTION(invalid_operation()
					<< msg_info("only one-to-many or many-to-one audio stream conversion supported."));

		std::wstring amerge;

		if (sink_->wanted_num_audio_streams() == 1 && !sink_->wanted_num_channels_per_stream())
		{
			output_pads.emplace_back(
					sink_->supported_samplerates(),
					sink_->supported_sample_formats(),
					std::vector<int64_t>({ av_get_default_channel_layout(total_num_channels) }));

			if (source_audio_streams_.size() > 1)
			{
				for (int i = 0; i < source_audio_streams_.size(); ++i)
					amerge += L"[a:" + boost::lexical_cast<std::wstring>(i) + L"]";

				amerge += L"amerge=inputs=" + boost::lexical_cast<std::wstring>(source_audio_streams_.size());
			}
		}

		std::wstring afilter = u16(afilter_);

		if (!amerge.empty())
		{
			afilter = prepend_filter(u16(afilter), amerge);
			afilter += L"[aout:0]";
		}

		audio_filter_.reset(new audio_filter(input_pads, output_pads, u8(afilter)));
	}

	void initialize_video_filter_if_needed(const AVFrame& av_frame)
	{
		bool changed = false;

		set_if_changed(changed, source_video_stream_.width, av_frame.width);
		set_if_changed(changed, source_video_stream_.height, av_frame.height);
		set_if_changed(changed, source_video_stream_.pixelformat, static_cast<AVPixelFormat>(av_frame.format));

		core::field_mode field_mode = core::field_mode::progressive;

		if (av_frame.interlaced_frame)
			field_mode = av_frame.top_field_first ? core::field_mode::upper : core::field_mode::lower;

		set_if_changed(changed, source_video_stream_.fieldmode, field_mode);

		if (changed)
			initialize_video_filter();
	}

	void initialize_video_filter()
	{
		if (source_video_stream_.fieldmode != core::field_mode::progressive && !filter::is_deinterlacing(u16(vfilter_)))
			vfilter_ = u8(append_filter(u16(vfilter_), L"YADIF=1:-1"));

		if (source_video_stream_.height == 480) // NTSC DV
		{
			auto pad_str = L"PAD=" + boost::lexical_cast<std::wstring>(source_video_stream_.width) + L":486:0:2:black";
			vfilter_ = u8(append_filter(u16(vfilter_), pad_str));
		}

		video_filter_.reset(new filter(
				source_video_stream_.width,
				source_video_stream_.height,
				1 / source_->framerate(),
				source_->framerate(),
				boost::rational<int>(1, 1), // TODO
				source_video_stream_.pixelformat,
				sink_->supported_pixel_formats(),
				vfilter_));
		sink_->framerate(framerate());
	}
};

spl::shared_ptr<struct ffmpeg_pipeline_backend> create_internal_pipeline()
{
	return spl::make_shared<ffmpeg_pipeline_backend_internal>();
}

}}
