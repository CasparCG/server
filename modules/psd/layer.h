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
* Author: Niklas P Andersson, niklas.p.andersson@svt.se
*/

#ifndef _PSDLAYER_H__
#define _PSDLAYER_H__

#pragma once

#include <vector>
#include <string>
#include <memory>
#include "util\bigendian_file_input_stream.h"

#include "image.h"
#include "misc.h"
#include "channel.h"
#include <boost/property_tree/ptree.hpp>

namespace caspar { namespace psd {

class layer;
typedef std::shared_ptr<layer> layer_ptr;

class layer
{
public:
	class layer_mask
	{
		friend class layer;
	public:

		bool enabled() { return (flags_ & 2) == 0; }
		bool linked() { return (flags_ & 1) == 0;  }
		bool inverted() { return (flags_ & 4) == 4; }

		void read_mask_data(BEFileInputStream&);

		char			mask_id_;
		image8bit_ptr	mask_;
		psd::rect<long> rect_;
		unsigned char	default_value_;
		unsigned char	flags_;
	};

	layer() : blend_mode_(InvalidBlendMode), link_group_id_(0), opacity_(255), baseClipping_(false), flags_(0), protection_flags_(0), masks_(0)
	{}

	static std::shared_ptr<layer> create(BEFileInputStream&);
	void populate(BEFileInputStream&);
	void read_channel_data(BEFileInputStream&);

	const std::wstring& name() const
	{
		return name_;
	}
	const psd::rect<long>& rect() const
	{
		return rect_;
	}

	unsigned char opacity() const
	{
		return opacity_;
	}

	unsigned short flags() const
	{
		return flags_;
	}

	bool visible() { return (flags_ & 2) == 0; }	//the (PSD file-format) documentation is is saying the opposite but what the heck
	bool is_position_protected() { return (protection_flags_& 4) == 4; }
	bool is_text() const;

	const boost::property_tree::wptree& text_data() const { return text_layer_info_; }

	const image8bit_ptr& image() const { return image_; }

	const layer_mask& mask_info() const { return mask_; }
	const image8bit_ptr& mask() const { return mask_.mask_; }

	int link_group_id() { return link_group_id_; }
	void set_link_group_id(int id) { link_group_id_ = id; }

private:
	channel_ptr get_channel(channel_type);
	void read_blending_ranges(BEFileInputStream&);

	caspar::psd::rect<long>			rect_;
	std::vector<channel_ptr>		channels_;
	blend_mode						blend_mode_;
	int								link_group_id_;
	unsigned char					opacity_;
	bool							baseClipping_;
	unsigned char					flags_;
	int								protection_flags_;
	std::wstring					name_;
	char							masks_;

	layer_mask						mask_;

	image8bit_ptr					image_;

	boost::property_tree::wptree	text_layer_info_;
};

}	//namespace psd
}	//namespace caspar

#endif	//_PSDLAYER_H__