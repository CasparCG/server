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

#include "../frame/frame_fwd.h"
#include "../frame/gpu_frame.h"
#include "../frame/frame_factory.h"

#include <boost/noncopyable.hpp>

#include <memory>

namespace caspar { namespace core {
	
class frame_producer : boost::noncopyable
{
public:
	virtual ~frame_producer(){}	
	virtual gpu_frame_ptr get_frame() = 0;
	virtual std::shared_ptr<frame_producer> get_following_producer() const { return nullptr; }
	virtual void set_leading_producer(const std::shared_ptr<frame_producer>&) {}
	virtual const frame_format_desc& get_frame_format_desc() const = 0;
	virtual void initialize(const frame_factory_ptr& factory) = 0;
};
typedef std::shared_ptr<frame_producer> frame_producer_ptr;

}}