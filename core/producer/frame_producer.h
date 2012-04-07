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

#pragma once

#include "../monitor/monitor.h"
#include "../video_format.h"

#include <common/forward.h>
#include <common/future_fwd.h>
#include <common/memory.h>
#include <common/enum_class.h>

#include <cstdint>
#include <limits>
#include <functional>
#include <string>
#include <type_traits>
#include <vector>

#include <boost/property_tree/ptree_fwd.hpp>

FORWARD1(caspar, class executor);

namespace caspar { namespace core {
	
// Interface
class frame_producer : public monitor::observable
{
	frame_producer(const frame_producer&);
	frame_producer& operator=(const frame_producer&);
public:

	// Static Members
	
	static const spl::shared_ptr<frame_producer>& empty();

	// Constructors

	frame_producer(){}
	virtual ~frame_producer(){}	

	// Methods	

	virtual class draw_frame					receive() = 0;
	virtual boost::unique_future<std::wstring>	call(const std::wstring& params) = 0;
	
	// monitor::observable

	virtual void subscribe(const monitor::observable::observer_ptr& o) = 0;
	virtual void unsubscribe(const monitor::observable::observer_ptr& o) = 0;

	// Properties
	

	virtual void								paused(bool value) = 0;
	virtual std::wstring						print() const = 0;
	virtual std::wstring						name() const = 0;
	virtual boost::property_tree::wptree		info() const = 0;
	virtual uint32_t							nb_frames() const = 0;
	virtual uint32_t							frame_number() const = 0;
	virtual class draw_frame					last_frame() = 0;
	virtual void								leading_producer(const spl::shared_ptr<frame_producer>&) {}  
};

class frame_producer_base : public frame_producer
{
public:
	frame_producer_base();
	virtual ~frame_producer_base(){}	

	// Methods	

	virtual boost::unique_future<std::wstring>	call(const std::wstring& params);
	
	// monitor::observable
	
	// Properties
	
	void						paused(bool value) override;	
	uint32_t					nb_frames() const override;
	uint32_t					frame_number() const override;
	virtual class draw_frame	last_frame() override;

private:
	virtual class draw_frame	receive() override;
	virtual class draw_frame	receive_impl() = 0;

	struct impl;
	friend struct impl;
	std::shared_ptr<impl> impl_;
};

typedef std::function<spl::shared_ptr<core::frame_producer>(const spl::shared_ptr<class frame_factory>&, const video_format_desc& format_desc, const std::vector<std::wstring>&)> producer_factory_t;
void register_producer_factory(const producer_factory_t& factory); // Not thread-safe.

spl::shared_ptr<core::frame_producer> create_producer(const spl::shared_ptr<frame_factory>&, const video_format_desc& format_desc, const std::vector<std::wstring>& params);
spl::shared_ptr<core::frame_producer> create_producer(const spl::shared_ptr<frame_factory>&, const video_format_desc& format_desc, const std::wstring& params);

spl::shared_ptr<core::frame_producer> create_destroy_proxy(spl::shared_ptr<core::frame_producer> producer);
		
}}
