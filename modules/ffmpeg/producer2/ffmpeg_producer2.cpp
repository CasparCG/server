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

#include "../StdAfx.h"

#include "ffmpeg_producer2.h"

#include "../producer/util/util.h"
#include "av_producer.h"

#include "../ffmpeg.h"
#include "../ffmpeg_error.h"

#include <common/param.h>
#include <common/diagnostics/graph.h>
#include <common/future.h>

#include <core/frame/draw_frame.h>
#include <core/help/help_repository.h>
#include <core/help/help_sink.h>
#include <core/producer/media_info/media_info.h>
#include <core/producer/framerate/framerate_producer.h>
#include <core/frame/frame_factory.h>

#include <tbb/concurrent_queue.h>
#include <tbb/atomic.h>

#include <boost/timer.hpp>

#include <atomic>
#include <future>
#include <queue>
#include <thread>

namespace caspar { namespace ffmpeg2 {

using namespace std::chrono_literals;

typedef std::vector<std::pair<std::string, std::string>> ffmpeg_options;

// HACK
std::wstring get_relative_or_original(
	const std::wstring& filename,
	const boost::filesystem::path& relative_to)
{
	boost::filesystem::path file(filename);
	auto result = file.filename().wstring();

	boost::filesystem::path current_path = file;

	while (true)
	{
		current_path = current_path.parent_path();

		if (boost::filesystem::equivalent(current_path, relative_to))
			break;

		if (current_path.empty())
			return filename;

		result = current_path.filename().wstring() + L"/" + result;
	}

	return result;
}

struct ffmpeg_producer : public core::frame_producer_base
{
	const std::wstring 									filename_;
	const std::wstring 									path_relative_to_media_ = get_relative_or_original(filename_, env::media_folder());
	spl::shared_ptr<core::frame_factory> 				frame_factory_;
	core::video_format_desc								format_desc_;

	AVProducer											producer_;
    
	core::monitor::subject  							monitor_subject_;
	core::constraints									constraints_;

    core::draw_frame                                    last_frame_ = core::draw_frame::late();
public:
	explicit ffmpeg_producer(
			spl::shared_ptr<core::frame_factory> frame_factory,
			core::video_format_desc format_desc,
			std::wstring filename,
			std::wstring vfilter,
			std::wstring afilter,
			boost::optional<int64_t> start,
			boost::optional<int64_t> duration,
			boost::optional<bool> loop)
		: format_desc_(format_desc)
		, filename_(filename)
		, frame_factory_(frame_factory)
		, producer_(frame_factory_, 
					format_desc_, 
					u8(filename), 
					u8(vfilter), 
					u8(afilter), 
					start, 
					duration, 
					loop)
	{
		if (producer_.width() > 0 && producer_.height() > 0) {
			constraints_.width.set(producer_.width());
			constraints_.height.set(producer_.height());
		}
	}

	int64_t to_frames(int64_t pts) const
	{
		return av_rescale_q(pts, AVRational{ 1, AV_TIME_BASE }, AVRational{ format_desc_.duration, format_desc_.time_scale });
	}

	int64_t from_frames(int64_t frames) const
	{
		return av_rescale_q(frames, AVRational{ format_desc_.duration, format_desc_.time_scale }, AVRational{ 1, AV_TIME_BASE });
	}
 
	// frame_producer

    core::draw_frame last_frame() override
    {
        if (producer_.get() != core::draw_frame::late() || producer_.next()) {
            last_frame_ = core::draw_frame::still(producer_.get());
        }

        return last_frame_;
    }

	core::draw_frame receive_impl() override
	{
        if (!producer_.next()) {
            return core::draw_frame::late();
        }

        auto number = to_frames(producer_.time());
        auto count = to_frames(producer_.duration());

        monitor_subject_
            << core::monitor::message("/file/time") % (number / format_desc_.fps) % (count / format_desc_.fps)
            << core::monitor::message("/file/frame") % static_cast<int32_t>(number) % static_cast<int32_t>(count)
            << core::monitor::message("/file/fps") % format_desc_.fps
            << core::monitor::message("/file/path") % path_relative_to_media_
            << core::monitor::message("/loop") % producer_.loop();

        last_frame_ = core::draw_frame::still(producer_.get());

		return producer_.get();
	}
	
	core::constraints& pixel_constraints() override
	{
		return constraints_;
	}

	uint32_t nb_frames() const override
	{
		return producer_.loop() ? std::numeric_limits<std::uint32_t>::max() : static_cast<uint32_t>(to_frames(producer_.time()));
	}

	std::future<std::wstring> call(const std::vector<std::wstring>& params) override
	{
        std::wstring result;

        std::wstring cmd = params.at(0);
        std::wstring value;
        if (params.size() > 1) {
            value = params.at(1);
        }

        if (boost::iequals(cmd, L"loop")) {
            if (!value.empty()) {
                producer_.loop(boost::lexical_cast<bool>(value));
            }

            result = boost::lexical_cast<std::wstring>(producer_.loop());
        } else if (boost::iequals(cmd, L"in") || boost::iequals(cmd, L"start")) {
            if (!value.empty()) {
                producer_.start(from_frames(boost::lexical_cast<std::int64_t>(value)));
            }

            result = boost::lexical_cast<std::wstring>(to_frames(producer_.start()));
        } else if (boost::iequals(cmd, L"out")) {
            if (!value.empty()) {
                producer_.duration(from_frames(boost::lexical_cast<std::int64_t>(value)) - producer_.start());
            }

            result = boost::lexical_cast<std::wstring>(to_frames(producer_.start() + producer_.duration()));
        } else if (boost::iequals(cmd, L"length")) {
            if (!value.empty()) {
                producer_.duration(from_frames(boost::lexical_cast<std::int64_t>(value)));
            }

            result = boost::lexical_cast<std::wstring>(to_frames(producer_.duration()));
        } else if (boost::iequals(cmd, L"seek") && !value.empty()) {
            int64_t seek;
            if (boost::iequals(value, L"rel")) {
                seek = producer_.time();
            } else if (boost::iequals(value, L"in")) {
                seek = producer_.start();
            } else if (boost::iequals(value, L"out")) {
                seek = producer_.start() + producer_.duration();
            } else if (boost::iequals(value, L"end")) {
                seek = producer_.duration();
            } else {
                seek = from_frames(boost::lexical_cast<std::int64_t>(value));
            }

            if (params.size() > 2) {
                seek += from_frames(boost::lexical_cast<std::int64_t>(params.at(2)));
            }

            producer_.seek(seek);

            result = boost::lexical_cast<std::wstring>(seek);
        } else {
            CASPAR_THROW_EXCEPTION(invalid_argument());
        }

        std::promise<std::wstring> promise;
        promise.set_value(result);
        return promise.get_future();
	}

	boost::property_tree::wptree info() const override
	{
		boost::property_tree::wptree pt;
        pt.add(L"type", L"ffmpeg-producer");
        pt.add(L"filename", filename_);
        pt.add(L"width", producer_.width());
        pt.add(L"height", producer_.height());
        pt.add(L"progressive", format_desc_.field_mode == core::field_mode::progressive);
        pt.add(L"fps", format_desc_.fps);
        pt.add(L"loop", producer_.loop());
        pt.add(L"file-frame-number", to_frames(producer_.time()));
        pt.add(L"file-nb-frames", to_frames(producer_.duration()));

        return pt;
	}

	std::wstring print() const override
	{
        auto number = to_frames(producer_.time());
        auto count = to_frames(producer_.duration());

		return L"ffmpeg[" + 
			filename_ + L"|" + 
			boost::lexical_cast<std::wstring>(number) + L"/" + 
			boost::lexical_cast<std::wstring>(count) + 
			L"]";
	}

	std::wstring name() const override
	{
		return L"ffmpeg";
	}

	core::monitor::subject& monitor_output() override
	{
		return monitor_subject_;
	}
};

spl::shared_ptr<core::frame_producer> create_producer(
		const core::frame_producer_dependencies& dependencies,
		const std::vector<std::wstring>& params,
		const spl::shared_ptr<core::media_info_repository>& info_repo)
{
	auto file_or_url = params.at(0);

	if (!boost::contains(file_or_url, L"://")) {
		file_or_url = ffmpeg::probe_stem(env::media_folder() + L"/" + file_or_url, false);
	}

	if (file_or_url.empty()) {
		return core::frame_producer::empty();
	}

	auto loop = contains_param(L"LOOP", params);

	auto in = get_param(L"SEEK", params, static_cast<uint32_t>(0)); // compatibility
	in = get_param(L"IN", params, in);

	auto out = get_param(L"LENGTH", params, std::numeric_limits<uint32_t>::max());
	if (out < std::numeric_limits<uint32_t>::max() - in)
		out += in;
	else
		out = std::numeric_limits<uint32_t>::max();
	out = get_param(L"OUT", params, out);

	auto filter_str = get_param(L"FILTER", params, L"");
	auto custom_channel_order = get_param(L"CHANNEL_LAYOUT", params, L"");

	boost::ireplace_all(filter_str, L"DEINTERLACE_BOB", L"YADIF=1:-1");
	boost::ireplace_all(filter_str, L"DEINTERLACE_LQ", L"SEPARATEFIELDS");
	boost::ireplace_all(filter_str, L"DEINTERLACE", L"YADIF=0:-1");

	ffmpeg_options vid_params;
	bool haveFFMPEGStartIndicator = false;
	for (size_t i = 0; i < params.size() - 1; ++i)
	{
		if (!haveFFMPEGStartIndicator && params[i] == L"--") {
			haveFFMPEGStartIndicator = true;
			continue;
		} if (haveFFMPEGStartIndicator)
		{
			auto name = u8(params.at(i++)).substr(1);
			auto value = u8(params.at(i));
			vid_params.push_back(std::make_pair(name, value));
		}
	}

	const auto in_tb = AVRational{ dependencies.format_desc.duration,  dependencies.format_desc.time_scale };
	const auto out_tb = AVRational{ 1, AV_TIME_BASE };

	boost::optional<std::int64_t> start;
	boost::optional<std::int64_t> duration;

	if (in != 0) {
		start = av_rescale_q(static_cast<int64_t>(in), in_tb, out_tb);
	}

	if (out != std::numeric_limits<uint32_t>::max()) {
		duration = av_rescale_q(static_cast<int64_t>(out - in), in_tb, out_tb);
	}

	// TODO
	custom_channel_order;
	vid_params;

	auto vfilter = get_param(L"VF", params, filter_str);
	auto afilter = get_param(L"AF", params, get_param(L"FILTER", params, L""));

	auto producer = spl::make_shared<ffmpeg_producer>(
		dependencies.frame_factory,
		dependencies.format_desc,
		file_or_url,
		vfilter,
		afilter,
		start,
		duration,
		loop);

	return core::create_destroy_proxy(std::move(producer));
}

}}
