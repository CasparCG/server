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

#include "stdafx.h"

#include "thumbnail_generator.h"

#include <iostream>
#include <iterator>
#include <set>

#include <boost/thread.hpp>
#include <boost/range/algorithm/transform.hpp>
#include <boost/algorithm/string/predicate.hpp>

#include <tbb/atomic.h>

#include "producer/frame_producer.h"
#include "consumer/frame_consumer.h"
#include "mixer/mixer.h"
#include "video_format.h"
#include "producer/frame/basic_frame.h"
#include "producer/frame/frame_transform.h"

namespace caspar { namespace core {

std::wstring get_relative_without_extension(
		const boost::filesystem::wpath& file,
		const boost::filesystem::wpath& relative_to)
{
	auto result = file.stem();
		
	boost::filesystem::wpath current_path = file;

	while (true)
	{
		current_path = current_path.parent_path();

		if (boost::filesystem::equivalent(current_path, relative_to))
			break;

		if (current_path.empty())
			throw std::runtime_error("File not relative to folder");

		result = current_path.filename() + L"/" + result;
	}

	return result;
}

struct thumbnail_output : public mixer::target_t
{
	tbb::atomic<int> sleep_millis;
	std::function<void (const safe_ptr<read_frame>& frame)> on_send;

	thumbnail_output(int sleep_millis)
	{
		this->sleep_millis = sleep_millis;
	}

	void send(const std::pair<safe_ptr<read_frame>, std::shared_ptr<void>>& frame_and_ticket)
	{
		int current_sleep = sleep_millis;

		if (current_sleep > 0)
			boost::this_thread::sleep(boost::posix_time::milliseconds(current_sleep));

		on_send(frame_and_ticket.first);
		on_send = nullptr;
	}
};

struct thumbnail_generator::implementation
{
private:
	boost::filesystem::wpath media_path_;
	boost::filesystem::wpath thumbnails_path_;
	int width_;
	int height_;
	safe_ptr<ogl_device> ogl_;
	safe_ptr<diagnostics::graph> graph_;
	video_format_desc format_desc_;
	safe_ptr<thumbnail_output> output_;
	safe_ptr<mixer> mixer_;
	thumbnail_creator thumbnail_creator_;
	filesystem_monitor::ptr monitor_;
public:
	implementation(
			filesystem_monitor_factory& monitor_factory,
			const boost::filesystem::wpath& media_path,
			const boost::filesystem::wpath& thumbnails_path,
			int width,
			int height,
			const video_format_desc& render_video_mode,
			const safe_ptr<ogl_device>& ogl,
			int generate_delay_millis,
			const thumbnail_creator& thumbnail_creator)
		: media_path_(media_path)
		, thumbnails_path_(thumbnails_path)
		, width_(width)
		, height_(height)
		, ogl_(ogl)
		, format_desc_(render_video_mode)
		, output_(new thumbnail_output(generate_delay_millis))
		, mixer_(new mixer(graph_, output_, format_desc_, ogl))
		, thumbnail_creator_(thumbnail_creator)
		, monitor_(monitor_factory.create(
				media_path,
				ALL,
				true,
				[this] (filesystem_event event, const boost::filesystem::wpath& file)
				{
					this->on_file_event(event, file);
				},
				[this] (const std::set<boost::filesystem::wpath>& initial_files) 
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

	void on_initial_files(const std::set<boost::filesystem::wpath>& initial_files)
	{
		using namespace boost::filesystem;

		std::set<std::wstring> relative_without_extensions;
		boost::transform(
				initial_files,
				std::insert_iterator<std::set<std::wstring>>(
						relative_without_extensions,
						relative_without_extensions.end()),
				boost::bind(&get_relative_without_extension, _1, media_path_));

		for (wrecursive_directory_iterator iter(thumbnails_path_); iter != wrecursive_directory_iterator(); ++iter)
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

		for (wdirectory_iterator iter(folder); iter != wdirectory_iterator(); ++iter)
		{
			auto stem = iter->path().stem();

			if (boost::iequals(stem, base_file.filename()))
				monitor_->reemmit(iter->path());
		}
	}

	void generate_all()
	{
		monitor_->reemmit_all();
	}

	void on_file_event(filesystem_event event, const boost::filesystem::wpath& file)
	{
		switch (event)
		{
		case CREATED:
			if (needs_to_be_generated(file))
				generate_thumbnail(file);

			break;
		case MODIFIED:
			generate_thumbnail(file);

			break;
		case REMOVED:
			auto relative_without_extension = get_relative_without_extension(file, media_path_);
			boost::filesystem::remove(thumbnails_path_ / (relative_without_extension + L".png"));

			break;
		}
	}

	bool needs_to_be_generated(const boost::filesystem::wpath& file)
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

	void generate_thumbnail(const boost::filesystem::wpath& file)
	{
		auto media_file = get_relative_without_extension(file, media_path_);
		auto png_file = thumbnails_path_ / (media_file + L".png");
		boost::promise<void> thumbnail_ready;

		{
			auto producer = frame_producer::empty();

			try
			{
				producer = create_thumbnail_producer(mixer_, media_file);
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
			output_->on_send = [this, &png_file] (const safe_ptr<read_frame>& frame)
			{
				thumbnail_creator_(frame, format_desc_, png_file, width_, height_);
			};

			std::map<int, safe_ptr<basic_frame>> frames;
			auto raw_frame = basic_frame::empty();

			try
			{
				raw_frame = producer->create_thumbnail_frame();
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

			if (raw_frame == basic_frame::empty()
					|| raw_frame == basic_frame::empty()
					|| raw_frame == basic_frame::eof()
					|| raw_frame == basic_frame::late())
			{
				CASPAR_LOG(debug) << L"No thumbnail generated for " << media_file;
				return;
			}

			auto transformed_frame = make_safe<basic_frame>(raw_frame);
			transformed_frame->get_frame_transform().fill_scale[0] = static_cast<double>(width_) / format_desc_.width;
			transformed_frame->get_frame_transform().fill_scale[1] = static_cast<double>(height_) / format_desc_.height;
			frames.insert(std::make_pair(0, transformed_frame));

			std::shared_ptr<void> ticket(nullptr, [&thumbnail_ready](void*)
			{
				thumbnail_ready.set_value();
			});

			mixer_->send(std::make_pair(frames, ticket));
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
		const boost::filesystem::wpath& media_path,
		const boost::filesystem::wpath& thumbnails_path,
		int width,
		int height,
		const video_format_desc& render_video_mode,
		const safe_ptr<ogl_device>& ogl,
		int generate_delay_millis,
		const thumbnail_creator& thumbnail_creator)
		: impl_(new implementation(
				monitor_factory,
				media_path,
				thumbnails_path,
				width, height,
				render_video_mode,
				ogl,
				generate_delay_millis,
				thumbnail_creator))
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
