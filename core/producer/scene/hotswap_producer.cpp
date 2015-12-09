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

#include "../../StdAfx.h"

#include "hotswap_producer.h"

#include "../frame_producer.h"
#include "../../frame/draw_frame.h"

namespace caspar { namespace core {

struct hotswap_producer::impl
{
	spl::shared_ptr<monitor::subject>			monitor_subject;
	binding<std::shared_ptr<frame_producer>>	producer;
	constraints									size;

	impl(int width, int height, bool auto_size)
		: size(width, height)
	{
		producer.on_change([=]
		{
			if (auto_size)
			{
				size.width.bind(producer.get()->pixel_constraints().width);
				size.height.bind(producer.get()->pixel_constraints().height);
			}

			producer.get()->monitor_output().attach_parent(monitor_subject);
		});
	}
};

hotswap_producer::hotswap_producer(int width, int height, bool auto_size)
	: impl_(new impl(width, height, auto_size))
{
}

hotswap_producer::~hotswap_producer()
{
}

draw_frame hotswap_producer::receive_impl()
{
	auto producer = impl_->producer.get();

	if (producer)
		return producer->receive();
	else
		return draw_frame::empty();
}

constraints& hotswap_producer::pixel_constraints()
{
	return impl_->size;
}
	
std::wstring hotswap_producer::print() const
{
	auto producer = impl_->producer.get();
	return L"hotswap[" + (producer ? producer->print() : L"") + L"]";
}

std::wstring hotswap_producer::name() const
{
	return L"hotswap";
}
	
boost::property_tree::wptree hotswap_producer::info() const
{
	boost::property_tree::wptree info;
	info.add(L"type", L"hotswap");

	auto producer = impl_->producer.get();

	if (producer)
		info.add(L"producer", producer->print());

	return info;
}

monitor::subject& hotswap_producer::monitor_output()
{
	return *impl_->monitor_subject;
}

variable& hotswap_producer::get_variable(const std::wstring& name)
{
	auto producer = impl_->producer.get();

	if (producer)
		return producer->get_variable(name);
	else
		return frame_producer_base::get_variable(name);
}

const std::vector<std::wstring>& hotswap_producer::get_variables() const
{
	auto producer = impl_->producer.get();

	if (producer)
		return producer->get_variables();
	else
		return frame_producer_base::get_variables();
}

binding<std::shared_ptr<frame_producer>>& hotswap_producer::producer()
{
	return impl_->producer;
}

}}
