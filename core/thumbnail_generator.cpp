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
*/

#include "StdAfx.h"

#include "thumbnail_generator.h"

#include <iostream>
#include <iterator>
#include <set>
#include <future>

#include <boost/thread.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/filesystem.hpp>

#include <tbb/atomic.h>

#include <common/diagnostics/graph.h>

#include "producer/frame_producer.h"
#include "consumer/frame_consumer.h"
#include "mixer/mixer.h"
#include "mixer/image/image_mixer.h"
#include "video_format.h"
#include "frame/frame.h"
#include "frame/draw_frame.h"
#include "frame/frame_transform.h"
#include "frame/audio_channel_layout.h"
#include "producer/media_info/media_info.h"
#include "producer/media_info/media_info_repository.h"

namespace caspar { namespace core {

std::wstring get_relative_without_extension(
		const boost::filesystem::path& file,
		const boost::filesystem::path& relative_to)
{
	auto result = file.stem();

	boost::filesystem::path current_path = file;

	while (true)
	{
		current_path = current_path.parent_path();

		if (boost::filesystem::equivalent(current_path, relative_to))
			break;

		if (current_path.empty())
			throw std::runtime_error("File not relative to folder");

		result = current_path.filename() / result;
	}

	return result.wstring();
}

struct thumbnail_output
{
	tbb::atomic<int> sleep_millis;
	std::function<void (const_frame)> on_send;

	thumbnail_output(int sleep_millis)
	{
		this->sleep_millis = sleep_millis;
	}

	void send(const_frame frame, std::shared_ptr<void> frame_and_ticket)
	{
		int current_sleep = sleep_millis;

		if (current_sleep > 0)
			boost::this_thread::sleep_for(boost::chrono::milliseconds(current_sleep));

		on_send(std::move(frame));
		on_send = nullptr;
	}
};

struct thumbnail_generator::impl
{
private:
	boost::filesystem::path							media_path_;
	boost::filesystem::path							thumbnails_path_;
	int												width_;
	int												height_;
	spl::shared_ptr<image_mixer>					image_mixer_;
	spl::shared_ptr<diagnostics::graph>				graph_;
	video_format_desc								format_desc_;
	spl::unique_ptr<thumbnail_output>				output_;
	mixer											mixer_;
	thumbnail_creator								thumbnail_creator_;
	spl::shared_ptr<media_info_repository>			media_info_repo_;
	spl::shared_ptr<const frame_producer_registry>	producer_registry_;
	bool											mipmap_;
	filesystem_monitor::ptr							monitor_;
public:
	impl(
			filesystem_monitor_factory& monitor_factory,
			const boost::filesystem::path& media_path,
			const boost::filesystem::path& thumbnails_path,
			int width,
			int height,
			const video_format_desc& render_video_mode,
			std::unique_ptr<image_mixer> image_mixer,
			int generate_delay_millis,
			const thumbnail_creator& thumbnail_creator,
			spl::shared_ptr<media_info_repository> media_info_repo,
			spl::shared_ptr<const frame_producer_registry> producer_registry,
			bool mipmap)
		: media_path_(media_path)
		, thumbnails_path_(thumbnails_path)
		, width_(width)
		, height_(height)
		, image_mixer_(std::move(image_mixer))
		, format_desc_(render_video_mode)
		, output_(spl::make_unique<thumbnail_output>(generate_delay_millis))
		, mixer_(0, graph_, image_mixer_)
		, thumbnail_creator_(thumbnail_creator)
		, media_info_repo_(std::move(media_info_repo))
		, producer_registry_(std::move(producer_registry))
		, monitor_(monitor_factory.create(
				media_path,
				filesystem_event::ALL,
				true,
				[this] (filesystem_event event, const boost::filesystem::path& file)
				{
					this->on_file_event(event, file);
				},
				[this] (const std::set<boost::filesystem::path>& initial_files)
				{
					this->on_initial_files(initial_files);
				}))
	{
		graph_->set_text(L"thumbnail-channel");
		graph_->auto_reset();
		diagnostics::register_graph(graph_);
		//monitor_->initial_scan_completion().get();
		//output_->sleep_millis = 2000;
	}

	void on_initial_files(const std::set<boost::filesystem::path>& initial_files)
	{
		using namespace boost::filesystem;

		std::set<std::wstring> relative_without_extensions;
		boost::transform(
				initial_files,
				std::insert_iterator<std::set<std::wstring>>(
						relative_without_extensions,
						relative_without_extensions.end()),
				boost::bind(&get_relative_without_extension, _1, media_path_));

		for (boost::filesystem::wrecursive_directory_iterator iter(thumbnails_path_); iter != boost::filesystem::wrecursive_directory_iterator(); ++iter)
		{
			auto& path = iter->path();

			if (!is_regular_file(path))
				continue;

			auto relative_without_extension = get_relative_without_extension(path, thumbnails_path_);
			bool no_corresponding_media_file = relative_without_extensions.find(relative_without_extension) 
					== relative_without_extensions.end();

			if (no_corresponding_media_file)
				remove(thumbnails_path_ / (relative_without_extension + L".png"));
		}
	}

	void generate(const std::wstring& media_file)
	{
		using namespace boost::filesystem;
		auto base_file = media_path_ / media_file;
		auto folder = base_file.parent_path();

		for (boost::filesystem::directory_iterator iter(folder); iter != boost::filesystem::directory_iterator(); ++iter)
		{
			auto stem = iter->path().stem();

			if (boost::iequals(stem.wstring(), base_file.filename().wstring()))
				monitor_->reemmit(iter->path());
		}
	}

	void generate_all()
	{
		monitor_->reemmit_all();
	}

	void on_file_event(filesystem_event event, const boost::filesystem::path& file)
	{
		switch (event)
		{
		case filesystem_event::CREATED:
			if (needs_to_be_generated(file))
				generate_thumbnail(file);

			break;
		case filesystem_event::MODIFIED:
			generate_thumbnail(file);

			break;
		case filesystem_event::REMOVED:
			auto relative_without_extension = get_relative_without_extension(file, media_path_);
			boost::filesystem::remove(thumbnails_path_ / (relative_without_extension + L".png"));
			media_info_repo_->remove(file.wstring());

			break;
		}
	}

	bool needs_to_be_generated(const boost::filesystem::path& file)
	{
		using namespace boost::filesystem;

		auto png_file = thumbnails_path_ / (get_relative_without_extension(file, media_path_) + L".png");

		if (!exists(png_file))
			return true;

		std::time_t media_file_mtime;

		try
		{
			media_file_mtime = last_write_time(file);
		}
		catch (...)
		{
			// Probably removed.
			return false;
		}

		try
		{
			return media_file_mtime != last_write_time(png_file);
		}
		catch (...)
		{
			// thumbnail probably removed.
			return true;
		}
	}

	void generate_thumbnail(const boost::filesystem::path& file)
	{
		auto media_file = get_relative_without_extension(file, media_path_);
		auto png_file = thumbnails_path_ / (media_file + L".png");
		std::promise<void> thumbnail_ready;

		{
			auto producer = frame_producer::empty();

			try
			{
				producer = producer_registry_->create_thumbnail_producer(
						frame_producer_dependencies(image_mixer_, { }, format_desc_, producer_registry_),
						media_file);
			}
			catch (const boost::thread_interrupted&)
			{
				throw;
			}
			catch (...)
			{
				CASPAR_LOG(debug) << L"Thumbnail producer failed to initialize for " << media_file;
				return;
			}

			if (producer == frame_producer::empty())
			{
				CASPAR_LOG(trace) << L"No appropriate thumbnail producer found for " << media_file;
				return;
			}

			boost::filesystem::create_directories(png_file.parent_path());
			output_->on_send = [this, &png_file] (const_frame frame)
			{
				thumbnail_creator_(frame, format_desc_, png_file, width_, height_);
			};

			std::map<int, draw_frame> frames;
			auto raw_frame = draw_frame::empty();

			try
			{
				raw_frame = producer->create_thumbnail_frame();
				media_info_repo_->remove(file.wstring());
				media_info_repo_->get(file.wstring());
			}
			catch (const boost::thread_interrupted&)
			{
				throw;
			}
			catch (...)
			{
				CASPAR_LOG(debug) << L"Thumbnail producer failed to create thumbnail for " << media_file;
				return;
			}

			if (raw_frame == draw_frame::empty()
					|| raw_frame == draw_frame::late())
			{
				CASPAR_LOG(debug) << L"No thumbnail generated for " << media_file;
				return;
			}

			auto transformed_frame = draw_frame(raw_frame);
			transformed_frame.transform().image_transform.fill_scale[0] = static_cast<double>(width_) / format_desc_.width;
			transformed_frame.transform().image_transform.fill_scale[1] = static_cast<double>(height_) / format_desc_.height;
			transformed_frame.transform().image_transform.use_mipmap = mipmap_;
			frames.insert(std::make_pair(0, transformed_frame));

			std::shared_ptr<void> ticket(nullptr, [&thumbnail_ready](void*) { thumbnail_ready.set_value(); });

			auto mixed_frame = mixer_(std::move(frames), format_desc_, audio_channel_layout(2, L"stereo", L""));

			output_->send(std::move(mixed_frame), ticket);
			ticket.reset();
		}
		thumbnail_ready.get_future().get();

		if (boost::filesystem::exists(png_file))
		{
			// Adjust timestamp to match source file.
			try
			{
				boost::filesystem::last_write_time(png_file, boost::filesystem::last_write_time(file));
				CASPAR_LOG(debug) << L"Generated thumbnail for " << media_file;
			}
			catch (...)
			{
				// One of the files was removed before the call to last_write_time.
			}
		}
		else
			CASPAR_LOG(debug) << L"No thumbnail generated for " << media_file;
	}
};

thumbnail_generator::thumbnail_generator(
		filesystem_monitor_factory& monitor_factory,
		const boost::filesystem::path& media_path,
		const boost::filesystem::path& thumbnails_path,
		int width,
		int height,
		const video_format_desc& render_video_mode,
		std::unique_ptr<image_mixer> image_mixer,
		int generate_delay_millis,
		const thumbnail_creator& thumbnail_creator,
		spl::shared_ptr<media_info_repository> media_info_repo,
		spl::shared_ptr<const frame_producer_registry> producer_registry,
		bool mipmap)
		: impl_(new impl(
				monitor_factory,
				media_path,
				thumbnails_path,
				width, height,
				render_video_mode,
				std::move(image_mixer),
				generate_delay_millis,
				thumbnail_creator,
				media_info_repo,
				producer_registry,
				mipmap))
{
}

thumbnail_generator::~thumbnail_generator()
{
}

void thumbnail_generator::generate(const std::wstring& media_file)
{
	impl_->generate(media_file);
}

void thumbnail_generator::generate_all()
{
	impl_->generate_all();
}

}}
