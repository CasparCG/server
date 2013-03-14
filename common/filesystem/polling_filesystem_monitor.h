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

#include "filesystem_monitor.h"

namespace caspar {

/**
 * A portable filesystem monitor implementation which periodically polls the
 * filesystem for changes. Will not react instantly but never misses any
 * changes.
 * <p>
 * Will create a dedicated thread for each monitor created.
 */
class polling_filesystem_monitor_factory : public filesystem_monitor_factory
{
public:
	/**
	 * Constructor.
	 *
	 * @param scan_interval_millis The number of milliseconds between each
	 *                             scheduled scan. Lower values lowers the reaction
	 *                             time but causes more I/O.
	 */
	polling_filesystem_monitor_factory(int scan_interval_millis = 5000);
	virtual ~polling_filesystem_monitor_factory();
	virtual filesystem_monitor::ptr create(
			const boost::filesystem::wpath& folder_to_watch,
			filesystem_event events_of_interest_mask,
			bool report_already_existing,
			const filesystem_monitor_handler& handler,
			const initial_files_handler& initial_files_handler);
private:
	struct implementation;
	safe_ptr<implementation> impl_;
};

}
