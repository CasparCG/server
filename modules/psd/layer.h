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
#include "common/memory.h"
#include "util\bigendian_file_input_stream.h"

#include "image.h"
#include "misc.h"
#include "channel.h"
#include <boost/property_tree/ptree_fwd.hpp>

namespace caspar { namespace psd {

class layer;
typedef std::shared_ptr<layer> layer_ptr;
class psd_document;

class layer
{
	struct impl;
	spl::shared_ptr<impl> impl_;

public:
	class layer_mask_info
	{
		friend struct layer::impl;

		void read_mask_data(BEFileInputStream&);

		image8bit_ptr	bitmap_;
		unsigned char	default_value_;
		unsigned char	flags_;
		char			mask_id_;
		rect<long>		rect_;

	public:
		bool enabled() const { return (flags_ & 2) == 0; }
		bool linked() const { return (flags_ & 1) == 0;  }
		bool inverted() const { return (flags_ & 4) == 4; }

		const point<long>& location() const { return rect_.location; }
		const image8bit_ptr& bitmap() const { return bitmap_; }
	};

	layer();

	void populate(BEFileInputStream&, const psd_document&);
	void read_channel_data(BEFileInputStream&);

	const std::wstring& name() const;
	unsigned char opacity() const;
	unsigned short sheet_color() const;
	bool is_visible();
	bool is_position_protected();

	float text_scale() const;
	bool is_text() const;
	const boost::property_tree::wptree& text_data() const;

	bool is_solid() const;
	color<unsigned char> solid_color() const;

	bool has_timeline() const;
	const boost::property_tree::wptree& timeline_data() const;

	const point<long>& location() const;
	const image8bit_ptr& bitmap() const;

	int link_group_id() const;
	void set_link_group_id(int id);
};

}	//namespace psd
}	//namespace caspar

#endif	//_PSDLAYER_H__