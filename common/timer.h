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

#include <boost/chrono/system_clocks.hpp>

namespace caspar {

// Replacement of boost::timer because it uses std::clock which has very low resolution in linux.
class timer
{
	boost::int_least64_t _start_time;
public:
	timer()
	{
		_start_time = now();
	}

	void restart()
	{
		_start_time = now();
	}

	double elapsed() const
	{
		return static_cast<double>(now() - _start_time) / 1000.0;
	}
private:
	static boost::int_least64_t now()
	{
		using namespace boost::chrono;

		return duration_cast<milliseconds>(high_resolution_clock::now().time_since_epoch()).count();
	}
};

}
