/*
* Copyright 2013 Sveriges Television AB http://casparcg.com/
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

#include "../../stdafx.h"

#include "audio_util.h"

#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/foreach.hpp>
#include <boost/assign.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/property_tree/exceptions.hpp>

namespace caspar { namespace core {

channel_layout::channel_layout()
	: num_channels(0)
{
}

bool channel_layout::operator==(const channel_layout& other) const
{
	return channel_names == other.channel_names
			&& num_channels == other.num_channels;
}

int channel_layout::channel_index(const std::wstring& channel_name) const
{
	auto iter =
			std::find(channel_names.begin(), channel_names.end(), channel_name);

	if (iter == channel_names.end())
		return -1;

	return iter - channel_names.begin();
}

bool channel_layout::has_channel(const std::wstring& channel_name) const
{
	return channel_index(channel_name) != -1;
}

bool channel_layout::no_channel_names() const
{
	return channel_names.empty();
}

mix_config::mix_config()
	: strategy(add)
{
}

bool needs_rearranging(
		const channel_layout& source, const channel_layout& destination)
{
	if ((source.no_channel_names() || destination.no_channel_names())
			&& source.num_channels == destination.num_channels)
		return false;

	return !(source == destination);
}

struct channel_layout_repository::impl
{
	std::map<std::wstring, const channel_layout> layouts;
	boost::mutex mutex;
};

channel_layout_repository::channel_layout_repository()
	: impl_(new impl)
{
}

channel_layout_repository::~channel_layout_repository()
{
}

void channel_layout_repository::register_layout(const channel_layout& layout)
{
	boost::unique_lock<boost::mutex> lock(impl_->mutex);

	impl_->layouts.erase(layout.name);
	impl_->layouts.insert(std::make_pair(layout.name, layout));
}

const channel_layout& channel_layout_repository::get_by_name(
		const std::wstring& layout_name) const
{
	boost::unique_lock<boost::mutex> lock(impl_->mutex);

	auto iter = impl_->layouts.find(layout_name);

	if (iter == impl_->layouts.end())
		BOOST_THROW_EXCEPTION(invalid_argument()
				<< msg_info(narrow(layout_name) + " not found"));

	return iter->second;
}

channel_layout create_layout_from_string(
		const std::wstring& name,
		const std::wstring& layout_type,
		int num_channels,
		const std::wstring& channels)
{
	channel_layout layout;

	layout.name = boost::to_upper_copy(name);
	layout.layout_type = boost::to_upper_copy(layout_type);
	auto upper_channels = boost::to_upper_copy(channels);
	
	if (channels.length() > 0)
		boost::split(
				layout.channel_names,
				upper_channels,
				boost::is_any_of(L"\t "),
				boost::token_compress_on);

	layout.num_channels = num_channels == -1
			? layout.channel_names.size() : num_channels;

	return layout;
}

channel_layout create_unspecified_layout(int num_channels)
{
	channel_layout layout;

	layout.name = L"UNORDERED" + boost::lexical_cast<std::wstring>(
			num_channels) + L"CH";
	layout.layout_type = L"UNORDERED";
	layout.num_channels = num_channels;

	return layout;
}

void register_default_channel_layouts(channel_layout_repository& repository)
{
	repository.register_layout(create_layout_from_string(
			L"mono",         L"1.0",           1, L"C"));
	repository.register_layout(create_layout_from_string(
			L"stereo",       L"2.0",           2, L"L R"));
	repository.register_layout(create_layout_from_string(
			L"dts",          L"5.1",           6, L"C L R Ls Rs LFE"));
	repository.register_layout(create_layout_from_string(
			L"dolbye",       L"5.1+stereomix", 8, L"L R C LFE Ls Rs Lmix Rmix"
	));
	repository.register_layout(create_layout_from_string(
			L"dolbydigital", L"5.1",           6, L"L C R Ls Rs LFE"));
	repository.register_layout(create_layout_from_string(
			L"smpte",        L"5.1",           6, L"L R C LFE Ls Rs"));
	repository.register_layout(create_layout_from_string(
			L"passthru",     L"16ch",         16, L""));
}

void parse_channel_layouts(
		channel_layout_repository& repository,
		const boost::property_tree::wptree& layouts_element)
{
	BOOST_FOREACH(auto& layout, layouts_element)
	{
		repository.register_layout(create_layout_from_string(
				layout.second.get<std::wstring>(L"name"),
				layout.second.get<std::wstring>(L"type"),
				layout.second.get<int>(L"num-channels"),
				layout.second.get<std::wstring>(L"channels")));
	}
}

channel_layout_repository& default_channel_layout_repository()
{
	static channel_layout_repository repository;

	return repository;
}

struct mix_config_repository::impl
{
	std::map<std::wstring, std::map<std::wstring, const mix_config>> configs;
	boost::mutex mutex;
};

mix_config_repository::mix_config_repository()
	: impl_(new impl)
{
}

mix_config_repository::~mix_config_repository()
{
}

void mix_config_repository::register_mix_config(const mix_config& config)
{
	boost::unique_lock<boost::mutex> lock(impl_->mutex);

	impl_->configs[config.from_layout_type].erase(config.to_layout_type);
	impl_->configs[config.from_layout_type].insert(
			std::make_pair(config.to_layout_type, config));
}

boost::optional<mix_config> mix_config_repository::get_mix_config(
		const std::wstring& from_layout_type,
		const std::wstring& to_layout_type) const
{
	boost::unique_lock<boost::mutex> lock(impl_->mutex);

	auto iter = impl_->configs[from_layout_type].find(to_layout_type);

	if (iter == impl_->configs[from_layout_type].end())
		return boost::optional<mix_config>();

	return iter->second;
}

mix_config create_mix_config_from_string(
		const std::wstring& from_layout_type,
		const std::wstring& to_layout_type,
		mix_config::mix_strategy strategy,
		const std::vector<std::wstring>& mappings)
{
	mix_config config;
	config.from_layout_type = boost::to_upper_copy(from_layout_type);
	config.to_layout_type = boost::to_upper_copy(to_layout_type);
	config.strategy = strategy;

	BOOST_FOREACH(auto& mapping, mappings)
	{
		auto upper_mapping = boost::to_upper_copy(mapping);
		std::vector<std::wstring> words;
		boost::split(
				words,
				upper_mapping,
				boost::is_any_of(L"\t "),
				boost::token_compress_on);

		if (words.size() != 3)
			BOOST_THROW_EXCEPTION(invalid_argument() << msg_info(
					"mix_config mapping string must have 3 tokens"));

		auto from = words.at(0);
		auto to = words.at(1);
		auto influence = boost::lexical_cast<double>(words.at(2));

		config.destination_ch_by_source_ch.insert(std::make_pair(
				from,
				mix_config::destination(to, influence)));
	}

	return config;
}

void register_default_mix_configs(mix_config_repository& repository)
{
	using namespace boost::assign;

	// From 1.0
	repository.register_mix_config(create_mix_config_from_string(
			L"1.0", L"2.0", mix_config::add, list_of
					(L"C L 1.0")
					(L"C R 1.0")
	));
	repository.register_mix_config(create_mix_config_from_string(
			L"1.0", L"5.1", mix_config::add, list_of
					(L"C L 1.0")
					(L"C R 1.0")
	));
	repository.register_mix_config(create_mix_config_from_string(
			L"1.0", L"5.1+stereomix", mix_config::add, list_of
					(L"C L    1.0")
					(L"C R    1.0")
					(L"C Lmix 1.0")
					(L"C Rmix 1.0")
	));
	// From 2.0
	repository.register_mix_config(create_mix_config_from_string(
			L"2.0", L"1.0", mix_config::add, list_of
					(L"L C 1.0")
					(L"R C 1.0")
	));
	repository.register_mix_config(create_mix_config_from_string(
			L"2.0", L"5.1", mix_config::add, list_of
					(L"L L    1.0")
					(L"R R    1.0")
	));
	repository.register_mix_config(create_mix_config_from_string(
			L"2.0", L"5.1+stereomix", mix_config::add, list_of
					(L"L L    1.0")
					(L"R R    1.0")
					(L"L Lmix 1.0")
					(L"R Rmix 1.0")
	));
	// From 5.1
	repository.register_mix_config(create_mix_config_from_string(
			L"5.1", L"1.0", mix_config::average, list_of
					(L"L  C 1.0")
					(L"R  C 1.0")
					(L"C  C 0.707")
					(L"Ls C 0.707")
					(L"Rs C 0.707")
	));
	repository.register_mix_config(create_mix_config_from_string(
			L"5.1", L"2.0", mix_config::average, list_of
					(L"L  L 1.0")
					(L"R  R 1.0")
					(L"C  L 0.707")
					(L"C  R 0.707")
					(L"Ls L 0.707")
					(L"Rs R 0.707")
	));
	repository.register_mix_config(create_mix_config_from_string(
			L"5.1", L"5.1+stereomix", mix_config::average, list_of
					(L"L   L   1.0")
					(L"R   R   1.0")
					(L"C   C   1.0")
					(L"Ls  Ls  1.0")
					(L"Rs  Rs  1.0")
					(L"LFE LFE 1.0")

					(L"L  Lmix 1.0")
					(L"R  Rmix 1.0")
					(L"C  Lmix 0.707")
					(L"C  Rmix 0.707")
					(L"Ls Lmix 0.707")
					(L"Rs Rmix 0.707")
	));
	// From 5.1+stereomix
	repository.register_mix_config(create_mix_config_from_string(
			L"5.1+stereomix", L"1.0", mix_config::add, list_of
					(L"Lmix C 1.0")
					(L"Rmix C 1.0")
	));
	repository.register_mix_config(create_mix_config_from_string(
			L"5.1+stereomix", L"2.0", mix_config::add, list_of
					(L"Lmix L 1.0")
					(L"Rmix R 1.0")
	));
	repository.register_mix_config(create_mix_config_from_string(
			L"5.1+stereomix", L"5.1", mix_config::add, list_of
					(L"L   L   1.0")
					(L"R   R   1.0")
					(L"C   C   1.0")
					(L"Ls  Ls  1.0")
					(L"Rs  Rs  1.0")
					(L"LFE LFE 1.0")
	));
}

void parse_mix_configs(
		mix_config_repository& repository, 
		const boost::property_tree::wptree& channel_mixings_element)
{
	BOOST_FOREACH(auto element, channel_mixings_element)
	{
		if (element.first != L"mix-config")
		{
			throw boost::property_tree::ptree_error(
				"Expected mix-config element");
		}

		std::vector<std::wstring> mappings;

		boost::transform(
				element.second.get_child(L"mappings"),
				std::insert_iterator<std::vector<std::wstring>>(
						mappings, mappings.begin()),
				[](
						const std::pair<std::wstring,
						boost::property_tree::wptree>& mapping)
				{
					return mapping.second.get_value<std::wstring>();
				});

		repository.register_mix_config(create_mix_config_from_string(
				element.second.get<std::wstring>(L"from"),
				element.second.get<std::wstring>(L"to"),
				boost::to_upper_copy(element.second.get<std::wstring>(
						L"mix", L"ADD")) == L"AVERAGE"
								? mix_config::average : mix_config::add,
				mappings));
	}
}

mix_config_repository& default_mix_config_repository()
{
	static mix_config_repository repository;

	return repository;
}

channel_layout create_custom_channel_layout(
		const std::wstring& custom_channel_order,
		const channel_layout_repository& repository)
{
	std::vector<std::wstring> splitted;
	boost::split(
			splitted,
			custom_channel_order,
			boost::is_any_of(L":"),
			boost::token_compress_on);

	if (splitted.size() == 1) // Named layout
	{
		try
		{
			return repository.get_by_name(splitted[0]);
		}
		catch (const std::exception&)
		{
			CASPAR_LOG_CURRENT_EXCEPTION();
			BOOST_THROW_EXCEPTION(caspar_exception() << msg_info(
					"CHANNEL_LAYOUT must be in a format like: "
					"\"5.1:L R C LFE Ls Rs\" or like \"SMPTE\""));
		}
	}

	if (splitted.size() != 2)
		BOOST_THROW_EXCEPTION(caspar_exception() << msg_info(
				"CHANNEL_LAYOUT must be in a format like: "
				"\"5.1:L R C LFE Ls Rs\" or like \"SMPTE\""));

	// Custom layout
	return create_layout_from_string(
			custom_channel_order,
			splitted[0],
			-1,
			splitted[1]);
}


}}
