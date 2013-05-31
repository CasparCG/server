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
* Author: Robert Nagy, ronag89@gmail.com
* Author: Helge Norberg, helge.norberg@svt.se
*/

#pragma once

#include <algorithm>
#include <vector>
#include <map>
#include <string>

#include <stdint.h>

#include <boost/range/iterator_range.hpp>
#include <boost/range/algorithm/copy.hpp>
#include <boost/range/combine.hpp>
#include <boost/range/adaptors.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/foreach.hpp>

#include <tbb/cache_aligned_allocator.h>

#include <common/exception/exceptions.h>
#include <common/utility/iterator.h>
#include <common/utility/string.h>
#include <common/memory/safe_ptr.h>

namespace caspar { namespace core {
	
template<typename T>
static std::vector<int8_t, tbb::cache_aligned_allocator<int8_t>> audio_32_to_24(const T& audio_data)
{	
	auto size		 = std::distance(std::begin(audio_data), std::end(audio_data));
	auto input8		 = reinterpret_cast<const int8_t*>(&(*std::begin(audio_data)));
	auto output8	 = std::vector<int8_t, tbb::cache_aligned_allocator<int8_t>>();
			
	output8.reserve(size*3);
	for(int n = 0; n < size; ++n)
	{
		output8.push_back(input8[n*4+1]);
		output8.push_back(input8[n*4+2]);
		output8.push_back(input8[n*4+3]);
	}

	return output8;
}

template<typename T>
static std::vector<int16_t, tbb::cache_aligned_allocator<int16_t>> audio_32_to_16(const T& audio_data)
{	
	auto size		 = std::distance(std::begin(audio_data), std::end(audio_data));
	auto input32	 = &(*std::begin(audio_data));
	auto output16	 = std::vector<int16_t, tbb::cache_aligned_allocator<int16_t>>();
			
	output16.reserve(size);
	for(int n = 0; n < size; ++n)
		output16.push_back((input32[n] >> 16) & 0xFFFF);

	return output16;
}


struct channel_layout
{
	std::wstring name;
	std::wstring layout_type;
	std::vector<std::wstring> channel_names;
	int num_channels;

	channel_layout();
	bool operator==(const channel_layout& other) const;
	int channel_index(const std::wstring& channel_name) const;
	bool has_channel(const std::wstring& channel_name) const;
	bool no_channel_names() const;

	static const channel_layout& stereo();
};

/**
 * A multichannel view of an audio buffer where the samples has to be
 * interleaved like this (given 4 channels):
 *
 * Memory Sample:   0  1  2  3  4  5  6  7  8  9  10 11
 * Temporal Sample: 0           1           2
 * Channel:         1  2  3  4  1  2  3  4  1  2  3  4
 *
 * Exposes each individual channel as an isolated iterable range.
 */
template<typename SampleT, typename Iter>
class multichannel_view
{
	Iter begin_;
	Iter end_;
	channel_layout channel_layout_;
	int num_channels_;
public:
	typedef position_based_skip_iterator<SampleT, constant_step_finder, Iter>
			iter_t;

	multichannel_view(
			const Iter& begin, 
			const Iter& end, 
			const channel_layout& channel_layout)
		: begin_(begin)
		, end_(end)
		, channel_layout_(channel_layout)
		, num_channels_(channel_layout.num_channels)
	{
	}

	multichannel_view(
			const Iter& begin,
			const Iter& end,
			const channel_layout& channel_layout,
			int num_channels)
		: begin_(begin)
		, end_(end)
		, channel_layout_(channel_layout)
		, num_channels_(num_channels)
	{
	}

	Iter raw_begin() const
	{
		return begin_;
	}

	Iter raw_end() const
	{
		return end_;
	}

	int num_channels() const
	{
		return num_channels_;
	}

	int num_samples() const
	{
		return std::distance(begin_, end_) / num_channels();
	}

	const channel_layout& channel_layout() const
	{
		return channel_layout_;
	}

	boost::iterator_range<iter_t> channel(int channel) const
	{
		auto start_position = begin_ + channel;

		return boost::iterator_range<iter_t>(
				iter_t(
						start_position,
						end_,
						constant_step_finder(
								num_channels(), num_samples() - 1)),
				iter_t(end_));
	}

	boost::iterator_range<iter_t> channel(
			const std::wstring& channel_name) const
	{
		auto it = std::find(
				channel_layout_.channel_names.begin(),
				channel_layout_.channel_names.end(),
				channel_name);

		if (it == channel_layout_.channel_names.end())
			BOOST_THROW_EXCEPTION(invalid_argument() 
					<< msg_info("channel not found " + narrow(channel_name)));

		auto index = it - channel_layout_.channel_names.begin();

		return channel(index);
	}
};

template<typename SampleT, typename Iter>
multichannel_view<SampleT, Iter> make_multichannel_view(
		const Iter& begin,
		const Iter& end,
		const channel_layout& channel_layout)
{
	return multichannel_view<SampleT, Iter>(begin, end, channel_layout);
}

template<typename SampleT, typename Iter>
multichannel_view<SampleT, Iter> make_multichannel_view(
		const Iter& begin,
		const Iter& end,
		const channel_layout& channel_layout,
		int num_channels)
{
	return multichannel_view<SampleT, Iter>(
			begin, end, channel_layout, num_channels);
}

struct mix_config
{
	struct destination
	{
		std::wstring channel_name;
		double influence;

		destination(const std::wstring& channel_name, double influence)
			: channel_name(channel_name), influence(influence)
		{
		}
	};

	enum mix_strategy
	{
		add,
		average
	};

	std::wstring from_layout_type;
	std::wstring to_layout_type;
	std::multimap<std::wstring, destination> destination_ch_by_source_ch;
	mix_strategy strategy;

	mix_config();
};

bool needs_rearranging(
		const channel_layout& source, const channel_layout& destination);

template<typename SrcView>
bool needs_rearranging(
		const SrcView& source,
		const channel_layout& destination,
		int destination_num_channels)
{
	return needs_rearranging(source.channel_layout(), destination) 
			|| source.num_channels() != destination_num_channels;
}

template<
	typename SrcSampleT,
	typename DstSampleT,
	typename SrcIter,
	typename DstIter>
void rearrange(
		const multichannel_view<SrcSampleT, SrcIter>& source,
		multichannel_view<DstSampleT, DstIter>& destination)
{
	if (source.channel_layout().no_channel_names()
			|| destination.channel_layout().no_channel_names())
	{
		int num_channels = std::min(
				source.channel_layout().num_channels,
				destination.channel_layout().num_channels);

		for (int i = 0; i < num_channels; ++i)
			boost::copy(source.channel(i), destination.channel(i).begin());
	}
	else
	{
		BOOST_FOREACH(
				auto& source_channel_name,
				source.channel_layout().channel_names)
		{
			if (source_channel_name.empty()
					|| !destination.channel_layout().has_channel(
							source_channel_name))
				continue;

			boost::copy(
					source.channel(source_channel_name),
					destination.channel(source_channel_name).begin());
		}
	}
}

template<typename SampleT>
struct add
{
	typedef SampleT result_type;

	result_type operator()(SampleT lhs, SampleT rhs) const
	{
		int64_t result = static_cast<int64_t>(lhs) + static_cast<int64_t>(rhs);

		if (result > std::numeric_limits<SampleT>::max())
			return std::numeric_limits<SampleT>::max();
		else if (result < std::numeric_limits<SampleT>::min())
			return std::numeric_limits<SampleT>::min();
		else
			return static_cast<SampleT>(result);
	}
};

template<typename SampleT>
struct attenuate
{
	typedef SampleT result_type;
	double volume;

	attenuate(double volume)
		: volume(volume)
	{
	}

	result_type operator()(SampleT sample) const
	{
		return static_cast<SampleT>(sample * volume);
	}
};

template<typename SampleT>
struct average
{
	typedef SampleT result_type;
	int num_samples_already;

	average(int num_samples_already)
		: num_samples_already(num_samples_already)
	{
	}

	result_type operator()(SampleT lhs, SampleT rhs) const
	{
		int64_t total = lhs * num_samples_already + rhs;

		return static_cast<SampleT>(total / (num_samples_already + 1));
	}
};

template<typename F>
struct tuple_to_args
{
	typedef typename F::result_type result_type;
	F func;

	tuple_to_args(F func)
		: func(func)
	{
	}
	
	template<typename T>
	result_type operator()(const T& tuple) const
	{
		return func(boost::get<0>(tuple), boost::get<1>(tuple));
	}
};

template<typename F>
tuple_to_args<F> make_tuple_to_args(const F& func)
{
	return tuple_to_args<F>(func);
}

template<
	typename SrcSampleT,
	typename DstSampleT,
	typename SrcIter,
	typename DstIter>
void rearrange_and_mix(
		const multichannel_view<SrcSampleT, SrcIter>& source,
		multichannel_view<DstSampleT, DstIter>& destination,
		const mix_config& config)
{
	using namespace boost;
	using namespace boost::adaptors;
	std::map<std::wstring, int> num_mixed_to_channel;

	BOOST_FOREACH(auto& elem, config.destination_ch_by_source_ch)
	{
		auto& source_channel_name = elem.first;
		auto& destination_channel_name = elem.second.channel_name;

		if (!source.channel_layout().has_channel(source_channel_name) ||
			!destination.channel_layout().has_channel(destination_channel_name))
			continue;

		auto source_channel = source.channel(source_channel_name);
		auto destination_channel =
				destination.channel(destination_channel_name);
		auto influence = elem.second.influence;

		if (num_mixed_to_channel[destination_channel_name] > 0)
		{ // mix
			if (config.strategy == mix_config::add)
			{
				if (influence == 1.0)
				{ // No need to attenuate
					copy(
						combine(destination_channel, source_channel)
								| transformed(make_tuple_to_args(
										add<SrcSampleT>())),
						destination_channel.begin());
				}
				else
				{
					copy(
						combine(
							destination_channel,
							source_channel | transformed(
								attenuate<SrcSampleT>(influence)))
									| transformed(make_tuple_to_args(
										add<SrcSampleT>())),
						destination_channel.begin());
				}
			}
			else
			{
				if (influence == 1.0)
				{ // No need to attenuate
					copy(
						combine(destination_channel, source_channel)
							| transformed(make_tuple_to_args(
								average<SrcSampleT>(num_mixed_to_channel[
										destination_channel_name]))),
						destination_channel.begin());
				}
				else
				{
					copy(
						combine(
							destination_channel,
							source_channel | transformed(
								attenuate<SrcSampleT>(influence)))
						| transformed(make_tuple_to_args(
							average<SrcSampleT>(num_mixed_to_channel[
									destination_channel_name]))),
						destination_channel.begin());
				}
			}
		}
		else
		{ // copy because the destination only contains zeroes
			if (influence == 1.0)
			{ // No need to attenuate
				copy(source_channel, destination_channel.begin());
			}
			else
			{
				copy(
					source_channel | transformed(attenuate<SrcSampleT>(
						influence)),
					destination_channel.begin());
			}
		}

		++num_mixed_to_channel[destination_channel_name];
	}
}

class channel_layout_repository
{
public:
	channel_layout_repository();
	~channel_layout_repository();
	void register_layout(const channel_layout& layout);
	const channel_layout& get_by_name(const std::wstring& layout_name) const;
private:
	struct impl;
	safe_ptr<impl> impl_;
};

channel_layout create_layout_from_string(
		const std::wstring& name,
		const std::wstring& layout_type,
		int num_channels,
		const std::wstring& channels);
channel_layout create_unspecified_layout(int num_channels);
void register_default_channel_layouts(channel_layout_repository& repository);
void parse_channel_layouts(
		channel_layout_repository& repository,
		const boost::property_tree::wptree& layouts_element);
channel_layout_repository& default_channel_layout_repository();

class mix_config_repository
{
public:
	mix_config_repository();
	~mix_config_repository();
	void register_mix_config(const mix_config& config);
	boost::optional<mix_config> get_mix_config(
			const std::wstring& from_layout_type,
			const std::wstring& to_layout_type) const;
private:
	struct impl;
	safe_ptr<impl> impl_;
};

mix_config create_mix_config_from_string(
		const std::wstring& from_layout_type,
		const std::wstring& to_layout_type,
		mix_config::mix_strategy strategy,
		const std::vector<std::wstring>& mappings);
void register_default_mix_configs(mix_config_repository& repository);
void parse_mix_configs(
		mix_config_repository& repository, 
		const boost::property_tree::wptree& channel_mixings_element);
mix_config_repository& default_mix_config_repository();

template<
	typename SrcSampleT,
	typename DstSampleT,
	typename SrcIter,
	typename DstIter>
bool rearrange_or_rearrange_and_mix(
		const multichannel_view<SrcSampleT, SrcIter>& source,
		multichannel_view<DstSampleT, DstIter>& destination,
		const mix_config_repository& repository)
{
	if (source.channel_layout().no_channel_names() 
			|| destination.channel_layout().no_channel_names() 
			|| source.channel_layout().layout_type
					== destination.channel_layout().layout_type)
	{
		rearrange(source, destination);

		return true;
	}
	else
	{
		auto mix_config = repository.get_mix_config(
				source.channel_layout().layout_type,
				destination.channel_layout().layout_type);

		if (!mix_config)
		{
			rearrange(source, destination);

			return false; // Non-satisfactory mixing, some channels might be
			              // lost
		}

		rearrange_and_mix(source, destination, *mix_config);

		return true;
	}
}

channel_layout create_custom_channel_layout(
		const std::wstring& custom_channel_order,
		const channel_layout_repository& repository);

}}
