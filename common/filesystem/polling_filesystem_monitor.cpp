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

#include "../stdafx.h"

#include "polling_filesystem_monitor.h"

#include <map>
#include <set>
#include <iostream>

#include <boost/thread.hpp>
#include <boost/foreach.hpp>
#include <boost/range/adaptor/map.hpp>
#include <boost/range/algorithm/copy.hpp>
#include <boost/filesystem/fstream.hpp>

#include <tbb/atomic.h>
#include <tbb/concurrent_queue.h>

#include "../concurrency/executor.h"

namespace caspar {

class exception_protected_handler
{
	filesystem_monitor_handler handler_;
public:
	exception_protected_handler(const filesystem_monitor_handler& handler)
		: handler_(handler)
	{
	}

	void operator()(filesystem_event event, const boost::filesystem::wpath& file)
	{
		try
		{
			boost::this_thread::interruption_point();
			handler_(event, file);
			boost::this_thread::interruption_point();
		}
		catch (const boost::thread_interrupted&)
		{
			throw;
		}
		catch (...)
		{
			CASPAR_LOG_CURRENT_EXCEPTION();
		}
	}
};

class directory_monitor
{
	bool report_already_existing_;
	boost::filesystem::wpath folder_;
	filesystem_event events_mask_;
	filesystem_monitor_handler handler_;
	initial_files_handler initial_files_handler_;
	bool first_scan_;
	std::map<boost::filesystem::wpath, std::time_t> files_;
	std::map<boost::filesystem::wpath, uintmax_t> being_written_sizes_;
public:
	directory_monitor(
			bool report_already_existing,
			const boost::filesystem::wpath& folder,
			filesystem_event events_mask,
			const filesystem_monitor_handler& handler,
			const initial_files_handler& initial_files_handler)
		: report_already_existing_(report_already_existing)
		, folder_(folder)
		, events_mask_(events_mask)
		, handler_(exception_protected_handler(handler))
		, initial_files_handler_(initial_files_handler)
		, first_scan_(true)
	{
	}

	void reemmit_all()
	{
		if ((events_mask_ & MODIFIED) == 0)
			return;

		BOOST_FOREACH(auto& file, files_)
			handler_(MODIFIED, file.first);
	}

	void reemmit(const boost::filesystem::wpath& file)
	{
		if ((events_mask_ & MODIFIED) == 0)
			return;

		if (files_.find(file) != files_.end() && boost::filesystem::exists(file))
			handler_(MODIFIED, file);
	}

	void scan(const boost::function<bool ()>& should_abort)
	{
		static const std::time_t NO_LONGER_WRITING_AGE = 3; // Assume std::time_t is expressed in seconds
		using namespace boost::filesystem;

		bool interested_in_removed = (events_mask_ & REMOVED) > 0;
		bool interested_in_created = (events_mask_ & CREATED) > 0;
		bool interested_in_modified = (events_mask_ & MODIFIED) > 0;

		std::set<wpath> removed_files;
		boost::copy(
				files_ | boost::adaptors::map_keys,
				std::insert_iterator<decltype(removed_files)>(removed_files, removed_files.end()));

		std::set<wpath> initial_files;

		for (wrecursive_directory_iterator iter(folder_); iter != wrecursive_directory_iterator(); ++iter)
		{
			if (should_abort())
				return;

			auto& path = iter->path();

			if (is_directory(path))
				continue;

			auto now = std::time(nullptr);
			std::time_t current_mtime;
			
			try
			{
				current_mtime = last_write_time(path);
			}
			catch (...)
			{
				// Probably removed, will be captured the next round.
				continue;
			}

			auto time_since_written_to = now - current_mtime;
			bool no_longer_being_written_to = time_since_written_to >= NO_LONGER_WRITING_AGE;
			auto previous_it = files_.find(path);
			bool already_known = previous_it != files_.end();

			if (already_known && no_longer_being_written_to)
			{
				bool modified = previous_it->second != current_mtime;

				if (modified && can_read_file(path))
				{
					if (interested_in_modified)
						handler_(MODIFIED, path);

					files_[path] = current_mtime;
					being_written_sizes_.erase(path);
				}
			}
			else if (no_longer_being_written_to && can_read_file(path))
			{
				if (interested_in_created && (report_already_existing_ || !first_scan_))
					handler_(CREATED, path);

				if (first_scan_)
					initial_files.insert(path);

				files_.insert(std::make_pair(path, current_mtime));
				being_written_sizes_.erase(path);
			}

			removed_files.erase(path);
		}

		BOOST_FOREACH(auto& path, removed_files)
		{
			files_.erase(path);
			being_written_sizes_.erase(path);

			if (interested_in_removed)
				handler_(REMOVED, path);
		}

		if (first_scan_)
			initial_files_handler_(initial_files);

		first_scan_ = false;
	}
private:
	bool can_read_file(const boost::filesystem::wpath& file)
	{
		boost::filesystem::wifstream stream(file);

		return stream.is_open();
	}
};

class polling_filesystem_monitor : public filesystem_monitor
{
	directory_monitor root_monitor_;
	boost::thread scanning_thread_;
	tbb::atomic<bool> running_;
	int scan_interval_millis_;
	boost::promise<void> initial_scan_completion_;
	tbb::concurrent_queue<boost::filesystem::wpath> to_reemmit_;
	tbb::atomic<bool> reemmit_all_;
public:
	polling_filesystem_monitor(
			const boost::filesystem::wpath& folder_to_watch,
			filesystem_event events_of_interest_mask,
			bool report_already_existing,
			int scan_interval_millis,
			const filesystem_monitor_handler& handler,
			const initial_files_handler& initial_files_handler)
		: root_monitor_(report_already_existing, folder_to_watch, events_of_interest_mask, handler, initial_files_handler)
		, scan_interval_millis_(scan_interval_millis)
	{
		running_ = true;
		reemmit_all_ = false;
		scanning_thread_ = boost::thread([this] { scanner(); });
	}

	virtual ~polling_filesystem_monitor()
	{
		running_ = false;
		scanning_thread_.interrupt();
		scanning_thread_.join();
	}

	virtual boost::unique_future<void> initial_files_processed()
	{
		return initial_scan_completion_.get_future();
	}

	virtual void reemmit_all()
	{
		reemmit_all_ = true;
	}

	virtual void reemmit(const boost::filesystem::wpath& file)
	{
		to_reemmit_.push(file);
	}
private:
	void scanner()
	{
		win32_exception::install_handler();
		detail::SetThreadName(GetCurrentThreadId(), "polling_filesystem_monitor");

		bool running = scan(false);
		initial_scan_completion_.set_value();

		if (running)
			while (scan(true));
	}

	bool scan(bool sleep)
	{
		try
		{
			if (sleep)
				boost::this_thread::sleep(boost::posix_time::milliseconds(scan_interval_millis_));

			if (reemmit_all_.fetch_and_store(false))
				root_monitor_.reemmit_all();
			else
			{
				boost::filesystem::wpath file;

				while (to_reemmit_.try_pop(file))
					root_monitor_.reemmit(file);
			}

			root_monitor_.scan([=] { return !running_; });
		}
		catch (const boost::thread_interrupted&)
		{
		}
		catch (...)
		{
			CASPAR_LOG_CURRENT_EXCEPTION();
		}

		return running_;
	}
};

struct polling_filesystem_monitor_factory::implementation
{
	int scan_interval_millis;

	implementation(int scan_interval_millis)
		: scan_interval_millis(scan_interval_millis)
	{
	}
};

polling_filesystem_monitor_factory::polling_filesystem_monitor_factory(int scan_interval_millis)
	: impl_(new implementation(scan_interval_millis))
{
}

polling_filesystem_monitor_factory::~polling_filesystem_monitor_factory()
{
}

filesystem_monitor::ptr polling_filesystem_monitor_factory::create(
		const boost::filesystem::wpath& folder_to_watch,
		filesystem_event events_of_interest_mask,
		bool report_already_existing,
		const filesystem_monitor_handler& handler,
		const initial_files_handler& initial_files_handler)
{
	return make_safe<polling_filesystem_monitor>(
			folder_to_watch,
			events_of_interest_mask,
			report_already_existing,
			impl_->scan_interval_millis,
			handler,
			initial_files_handler);
}

}
