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

#include "scene_cg_proxy.h"

#include <core/producer/frame_producer.h>

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>

#include <sstream>
#include <future>

namespace caspar { namespace core { namespace scene {

struct scene_cg_proxy::impl
{
	spl::shared_ptr<frame_producer> producer_;

	impl(spl::shared_ptr<frame_producer> producer)
		: producer_(std::move(producer))
	{
	}
};

scene_cg_proxy::scene_cg_proxy(spl::shared_ptr<frame_producer> producer)
	: impl_(new impl(std::move(producer)))
{
}

void scene_cg_proxy::add(
		int layer,
		const std::wstring& template_name,
		bool play_on_load,
		const std::wstring& start_from_label,
		const std::wstring& data)
{
	update(layer, data);

	if (play_on_load)
		play(layer);
}

void scene_cg_proxy::remove(int layer)
{
	impl_->producer_->call({ L"remove()" });
}

void scene_cg_proxy::play(int layer)
{
	impl_->producer_->call({ L"play()", L"intro" });
}

void scene_cg_proxy::stop(int layer, unsigned int mix_out_duration)
{
	impl_->producer_->call({ L"play()", L"outro" });
}

void scene_cg_proxy::next(int layer)
{
	impl_->producer_->call({ L"next()" });
}

void scene_cg_proxy::update(int layer, const std::wstring& data)
{
	if (data.empty())
		return;

	std::wstringstream stream(data);
	boost::property_tree::wptree root;
	boost::property_tree::read_xml(
			stream,
			root,
			boost::property_tree::xml_parser::trim_whitespace | boost::property_tree::xml_parser::no_comments);

	std::vector<std::wstring> parameters;

	for (auto value : root | witerate_children(L"templateData") | welement_context_iteration)
	{
		ptree_verify_element_name(value, L"componentData");

		auto id		= ptree_get<std::wstring>(value.second, L"<xmlattr>.id");
		auto val	= ptree_get<std::wstring>(value.second, L"data.<xmlattr>.value");

		parameters.push_back(std::move(id));
		parameters.push_back(std::move(val));
	}

	impl_->producer_->call(parameters);
}

std::wstring scene_cg_proxy::invoke(int layer, const std::wstring& label)
{
	if (boost::ends_with(label, L")"))
		return impl_->producer_->call({ L"play()", label.substr(0, label.find(L"(")) }).get();
	else
		return impl_->producer_->call({ L"play()", label }).get();
}

std::wstring scene_cg_proxy::description(int layer)
{
	return L"<scene producer />";
}

std::wstring scene_cg_proxy::template_host_info()
{
	return L"<scene producer />";
}

}}}
