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

#include <common/memory.h>

#include <core/mixer/audio/audio_mixer.h>

#include <boost/noncopyable.hpp>
#include <boost/optional.hpp>
#include <boost/property_tree/ptree_fwd.hpp>

#include <string>

namespace caspar { namespace core {

struct audio_channel_layout final
{
	int							num_channels;
	std::wstring				type;
	std::vector<std::wstring>	channel_order;

	audio_channel_layout(int num_channels, std::wstring type, const std::wstring& channel_order);
	std::vector<int> indexes_of(const std::wstring& channel_name) const;
	std::wstring print() const;
	static const audio_channel_layout& invalid();
private:
	audio_channel_layout();
};

bool operator==(const audio_channel_layout& lhs, const audio_channel_layout& rhs);
bool operator!=(const audio_channel_layout& lhs, const audio_channel_layout& rhs);

class audio_channel_layout_repository : boost::noncopyable
{
public:
	audio_channel_layout_repository();
	void register_layout(std::wstring name, audio_channel_layout layout);
	void register_all_layouts(const boost::property_tree::wptree& layouts);
	boost::optional<audio_channel_layout> get_layout(const std::wstring& name) const;
	static spl::shared_ptr<audio_channel_layout_repository> get_default();
private:
	struct impl;
	spl::shared_ptr<impl> impl_;
};

class audio_mix_config_repository : boost::noncopyable
{
public:
	audio_mix_config_repository();
	void register_config(
			const std::wstring& from_type,
			const std::vector<std::wstring>& to_types,
			const std::wstring& mix_config);
	void register_all_configs(const boost::property_tree::wptree& configs);
	boost::optional<std::wstring> get_config(const std::wstring& from_type, const std::wstring& to_type) const;
	static spl::shared_ptr<audio_mix_config_repository> get_default();
private:
	struct impl;
	spl::shared_ptr<impl> impl_;
};

// Implementation in ffmpeg module.
class audio_channel_remapper : boost::noncopyable
{
public:
	audio_channel_remapper(
			audio_channel_layout input_layout,
			audio_channel_layout output_layout,
			spl::shared_ptr<audio_mix_config_repository> mix_repo = audio_mix_config_repository::get_default());

	/**
	 * Perform downmix/upmix/rearranging of audio data if needed.
	 *
	 * @param input The input audio buffer.
	 *
	 * @return input if the input layout is the same as the output layout.
	 *         otherwise the mixed buffer (valid until the next call).
	 */
	audio_buffer mix_and_rearrange(audio_buffer input);
private:
	struct impl;
	spl::shared_ptr<impl> impl_;
};

}}
