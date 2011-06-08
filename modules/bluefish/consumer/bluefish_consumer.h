/*
* copyright (c) 2010 Sveriges Television AB <info@casparcg.com>
*
*  This file is part of CasparCG.
*
*    CasparCG is free software: you can redistribute it and/or modify
*    it under the terms of the GNU General Public License as published by
*    the Free Software Foundation, either version 3 of the License, or
*    (at your option) any later version.
*
*    CasparCG is distributed in the hope that it will be useful,
*    but WITHOUT ANY WARRANTY; without even the implied warranty of
*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*    GNU General Public License for more details.

*    You should have received a copy of the GNU General Public License
*    along with CasparCG.  If not, see <http://www.gnu.org/licenses/>.
*
*/
#pragma once

#include <core/consumer/frame_consumer.h>
#include <common/memory/safe_ptr.h>

#include <boost/property_tree/ptree.hpp>

#include <string>

namespace caspar { 
			
safe_ptr<core::frame_consumer> create_bluefish_consumer(const std::vector<std::wstring>& params);
safe_ptr<core::frame_consumer> create_bluefish_consumer(const boost::property_tree::ptree& ptree);

}