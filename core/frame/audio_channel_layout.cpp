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

#include "../StdAfx.h"

#include "audio_channel_layout.h"

#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/join.hpp>
#include <boost/range/algorithm/equal.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/property_tree/ptree.hpp>

#include <map>

namespace caspar { namespace core {

audio_channel_layout::audio_channel_layout()
	: num_channels(0)
{
}

audio_channel_layout::audio_channel_layout(int num_channels, std::wstring type_, const std::wstring& channel_order_)
	: num_channels(num_channels)
	, type(std::move(type_))
{
	if (num_channels < 1)
		CASPAR_THROW_EXCEPTION(invalid_argument() << msg_info(L"num_channels cannot be less than 1"));

	if (boost::contains(channel_order_, L"=") ||
		boost::contains(channel_order_, L"<") ||
		boost::contains(channel_order_, L"+") ||
		boost::contains(channel_order_, L"*") ||
		boost::contains(channel_order_, L"|"))
	{
		CASPAR_THROW_EXCEPTION(invalid_argument() << msg_info(
			channel_order_ + L" contains illegal characters =<+*| reserved for mix config syntax"));
	}

	boost::to_upper(type);
	boost::split(channel_order, channel_order_, boost::is_any_of(L" "), boost::algorithm::token_compress_on);

	if (channel_order.size() == 1 && channel_order.front().empty())
		channel_order.clear();

	if (channel_order.size() > num_channels)
		CASPAR_THROW_EXCEPTION(invalid_argument() << msg_info(
			channel_order_ + L" contains more than " + boost::lexical_cast<std::wstring>(num_channels)));
}

std::vector<int> audio_channel_layout::indexes_of(const std::wstring& channel_name) const
{
	std::vector<int> result;
	for (int i = 0; i < channel_order.size(); ++i)
		if (channel_name == channel_order.at(i))
			result.push_back(i);

	return result;
}

std::wstring audio_channel_layout::print() const
{
	auto channels = boost::join(channel_order, L" ");

	return L"[audio_channel_layout] num_channels=" + boost::lexical_cast<std::wstring>(num_channels) + L" type=" + type + L" channel_order=" + channels;
}

const audio_channel_layout& audio_channel_layout::invalid()
{
	static const audio_channel_layout instance;

	return instance;
}

bool operator==(const audio_channel_layout& lhs, const audio_channel_layout& rhs)
{
	return lhs.num_channels == rhs.num_channels
		&& boost::equal(lhs.channel_order, rhs.channel_order)
		&& lhs.type == rhs.type;
}

bool operator!=(const audio_channel_layout& lhs, const audio_channel_layout& rhs)
{
	return !(lhs == rhs);
}

struct audio_channel_layout_repository::impl
{
	mutable boost::mutex							mutex_;
	std::map<std::wstring, audio_channel_layout>	layouts_;
};

audio_channel_layout_repository::audio_channel_layout_repository()
	: impl_(new impl)
{
}

void audio_channel_layout_repository::register_layout(std::wstring name, audio_channel_layout layout)
{
	auto& self = *impl_;
	boost::lock_guard<boost::mutex> lock(self.mutex_);

	boost::to_upper(name);
	self.layouts_.insert(std::make_pair(std::move(name), std::move(layout)));
}

void audio_channel_layout_repository::register_all_layouts(const boost::property_tree::wptree& layouts)
{
	auto& self = *impl_;
	boost::lock_guard<boost::mutex> lock(self.mutex_);

	for (auto& layout : layouts | welement_context_iteration)
	{
		ptree_verify_element_name(layout, L"channel-layout");

		auto name			= ptree_get<std::wstring>(layout.second, L"<xmlattr>.name");
		auto type			= ptree_get<std::wstring>(layout.second, L"<xmlattr>.type");
		auto num_channels	= ptree_get<int>(layout.second, L"<xmlattr>.num-channels");
		auto channel_order	= layout.second.get<std::wstring>(L"<xmlattr>.channel-order", L"");

		boost::to_upper(name);
		self.layouts_.insert(std::make_pair(
				std::move(name),
				audio_channel_layout(num_channels, std::move(type), channel_order)));
	}
}

boost::optional<audio_channel_layout> audio_channel_layout_repository::get_layout(const std::wstring& name) const
{
	auto& self = *impl_;
	boost::lock_guard<boost::mutex> lock(self.mutex_);

	auto found = self.layouts_.find(boost::to_upper_copy(name));

	if (found == self.layouts_.end())
		return boost::none;

	return found->second;
}

spl::shared_ptr<audio_channel_layout_repository> audio_channel_layout_repository::get_default()
{
	static spl::shared_ptr<audio_channel_layout_repository> instance;

	return instance;
}

struct audio_mix_config_repository::impl
{
	mutable boost::mutex											mutex_;
	std::map<std::wstring, std::map<std::wstring, std::wstring>>	from_to_configs_;
};

audio_mix_config_repository::audio_mix_config_repository()
	: impl_(new impl)
{
}

void audio_mix_config_repository::register_config(
		const std::wstring& from_type,
		const std::vector<std::wstring>& to_types,
		const std::wstring& mix_config)
{
	auto& self = *impl_;
	boost::lock_guard<boost::mutex> lock(self.mutex_);
	
	for (auto& to_type : to_types)
		self.from_to_configs_[boost::to_upper_copy(from_type)][boost::to_upper_copy(to_type)] = mix_config;
}

void audio_mix_config_repository::register_all_configs(const boost::property_tree::wptree& configs)
{
	auto& self = *impl_;
	boost::lock_guard<boost::mutex> lock(self.mutex_);

	for (auto& config : configs | welement_context_iteration)
	{
		ptree_verify_element_name(config, L"mix-config");

		auto from_type		= ptree_get<std::wstring>(config.second, L"<xmlattr>.from-type");
		auto to_types_str	= ptree_get<std::wstring>(config.second, L"<xmlattr>.to-types");
		auto mix_config		= ptree_get<std::wstring>(config.second, L"<xmlattr>.mix");

		boost::to_upper(from_type);
		std::vector<std::wstring> to_types;
		boost::split(to_types, to_types_str, boost::is_any_of(L","), boost::algorithm::token_compress_off);

		for (auto& to_type : to_types)
		{
			boost::trim(to_type);

			if (to_type.empty())
				continue;

			boost::to_upper(to_type);
			self.from_to_configs_[from_type][to_type] = mix_config;
		}
	}
}

boost::optional<std::wstring> audio_mix_config_repository::get_config(
		const std::wstring& from_type,
		const std::wstring& to_type) const
{
	auto& self = *impl_;
	boost::lock_guard<boost::mutex> lock(self.mutex_);

	auto from_found = self.from_to_configs_.find(boost::to_upper_copy(from_type));

	if (from_found == self.from_to_configs_.end())
		return boost::none;

	auto to_found = from_found->second.find(boost::to_upper_copy(to_type));

	if (to_found == from_found->second.end())
		return boost::none;

	return to_found->second;
}

spl::shared_ptr<audio_mix_config_repository> audio_mix_config_repository::get_default()
{
	static spl::shared_ptr<audio_mix_config_repository> instance;

	return instance;
}

}}
