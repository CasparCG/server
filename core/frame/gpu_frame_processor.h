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

#include <memory>
#include <vector>

#include "frame_fwd.h"
#include "frame_factory.h"

#include <boost/signals2/signal.hpp>

namespace caspar { namespace core {

class gpu_frame_processor : public frame_factory,  boost::noncopyable
{
public:
	gpu_frame_processor(const frame_format_desc& format_desc);

	boost::signals2::connection subscribe(const std::function<void(const gpu_frame_ptr&)> func);
	void push(const std::vector<gpu_frame_ptr>& frames);

	gpu_frame_ptr create_frame(size_t width, size_t height);
private:
	struct implementation;
	std::shared_ptr<implementation> impl_;
};
typedef std::shared_ptr<gpu_frame_processor> gpu_frame_processor_ptr;

}}