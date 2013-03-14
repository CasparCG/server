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

#pragma once

#include <functional>
#include <set>

#include <boost/filesystem.hpp>
#include <boost/noncopyable.hpp>
#include <boost/thread/future.hpp>

#include "../memory/safe_ptr.h"

namespace caspar {

/**
 * A handle to an active filesystem monitor. Will stop scanning when destroyed.
 */
class filesystem_monitor : boost::noncopyable
{
public:
	typedef safe_ptr<filesystem_monitor> ptr;

	virtual ~filesystem_monitor() {}

	/**
	 * @return a future made available when the initially available files have been
	 *         processed.
	 */
	virtual boost::unique_future<void> initial_files_processed() = 0;

	/**
	 * Reemmit the already known files as MODIFIED events.
	 */
	virtual void reemmit_all() = 0;

	/**
	 * Reemmit a specific file as a MODIFIED event.
	 *
	 * @param file The file to reemmit.
	 */
	virtual void reemmit(const boost::filesystem::wpath& file) = 0;
};

/**
 * The possible filesystem events.
 */
enum filesystem_event
{
	CREATED = 1,
	REMOVED = 2,
	MODIFIED = 4,
	// Only used for describing a bitmask where all events are wanted. Never used when calling a handler.
	ALL = 7
};

/**
 * Handles filesystem events.
 * <p>
 * Will always be called from the same thread each time it is called.
 *
 * @param event Can be CREATED, REMOVED or MODIFIED.
 * @param file  The file affected.
 */
typedef std::function<void (filesystem_event event, const boost::filesystem::wpath& file)> filesystem_monitor_handler;

/**
 * Called when the initially available files has been found.
 * <p>
 * Will be called from the same thread as filesystem_monitor_handler is called.
 *
 * @param initial_files The files that were initially available.
 */
typedef std::function<void (const std::set<boost::filesystem::wpath>& initial_files)> initial_files_handler;

/**
 * Factory for creating filesystem monitors.
 */
class filesystem_monitor_factory : boost::noncopyable
{
public:
	virtual ~filesystem_monitor_factory() {}

	/**
	 * Create a new monitor.
	 *
	 * @param folder_to_watch         The folder to recursively watch.
	 * @param events_of_interest_mask A bitmask of the events to prenumerate on.
	 * @param report_already_existing Whether to initially report all files in
	 *                                the folder as CREATED or to only care
	 *                                about changes.
	 * @param handler                 The handler to call each time a change is
	 *                                detected (only events defined by
	 *                                events_of_interest_mask).
	 * @param initial_files_handler   The optional handler to call when the
	 *                                initially available files has been
	 *                                discovered.
	 *
	 * @return The filesystem monitor handle.
	 */
	virtual filesystem_monitor::ptr create(
			const boost::filesystem::wpath& folder_to_watch,
			filesystem_event events_of_interest_mask,
			bool report_already_existing,
			const filesystem_monitor_handler& handler,
			const initial_files_handler& initial_files_handler =
					[] (const std::set<boost::filesystem::wpath>&) { }) = 0;
};

}
