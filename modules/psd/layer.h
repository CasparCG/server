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

#pragma once

#include "util/bigendian_file_input_stream.h"
#include "image.h"
#include "misc.h"
#include "channel.h"

#include <boost/property_tree/ptree_fwd.hpp>

#include <vector>
#include <string>
#include <common/memory.h>

namespace caspar { namespace psd {

class layer;
typedef std::shared_ptr<layer> layer_ptr;
class psd_document;

class layer
{
	struct impl;
	spl::shared_ptr<impl> impl_;

public:
	class mask_info;

	class vector_mask_info
	{
		std::uint8_t			flags_;
		psd::rect<int>			rect_;
		std::vector<point<int>>	knots_;

		friend class layer::mask_info;
		bool populate(int length, bigendian_file_input_stream& stream, int doc_width, int doc_height);

	public:
		enum class flags {
			none = 0,
			inverted = 1,
			unlinked = 2,
			disabled = 4,
			unsupported = 128
		};

		vector_mask_info() : flags_(0)
		{}

		bool enabled() const { return (flags_ & static_cast<std::uint8_t>(flags::disabled)) == 0; }
		bool linked() const { return (flags_ & static_cast<std::uint8_t>(flags::unlinked)) == 0; }
		bool inverted() const { return (flags_ & static_cast<std::uint8_t>(flags::inverted)) == static_cast<std::uint8_t>(flags::inverted); }
		bool unsupported() const { return (flags_ & static_cast<std::uint8_t>(flags::unsupported)) == static_cast<std::uint8_t>(flags::unsupported); }

		bool empty() { return rect_.empty() && knots_.empty(); }

		const psd::rect<int>& rect() const { return rect_; }
		const std::vector<point<int>>& knots() const { return knots_; }
	};

	class mask_info
	{
		friend struct layer::impl;
		friend class layer::vector_mask_info;

		void read_mask_data(bigendian_file_input_stream&);
		void read_vector_mask_data(int length, bigendian_file_input_stream& stream, int doc_width, int doc_height)
		{
			vector_mask_.reset(new vector_mask_info);
			vector_mask_->populate(length, stream, doc_width, doc_height);
		}

		image8bit_ptr	bitmap_;
		std::uint8_t	default_value_;
		std::uint8_t	flags_;
		psd::rect<int>	rect_;

		std::unique_ptr<vector_mask_info> vector_mask_;
		std::unique_ptr<mask_info> total_mask_;

		void create_bitmap() {
			bitmap_ = std::make_shared<image8bit>(rect_.size.width, rect_.size.height, 1);
		}

	public:
		mask_info() : default_value_(0), flags_(0)
		{}

		bool enabled() const { return (flags_ & 2) == 0; }
		bool linked() const { return (flags_ & 1) == 0;  }
		bool inverted() const { return (flags_ & 4) == 4; }

		bool empty() const { return rect_.empty(); }
		
		bool has_vector() const { return (vector_mask_ && !vector_mask_->empty() && vector_mask_->enabled()); }
		bool has_bitmap() const { return (!vector_mask_ && !empty()) || (vector_mask_ && total_mask_); }
		const std::unique_ptr<vector_mask_info>& vector() const { return vector_mask_; }

		const psd::rect<int>& rect() const { return rect_; }
		const image8bit_ptr& bitmap() const { return bitmap_; }
	};

	layer();

	void populate(bigendian_file_input_stream&, const psd_document&);
	void read_channel_data(bigendian_file_input_stream&);

	const std::wstring& name() const;
	int opacity() const;
	caspar::core::blend_mode blend_mode() const;
	int sheet_color() const;
	bool is_visible();
	bool is_position_protected();

	const mask_info& mask() const;

	const psd::point<double>& text_pos() const;
	const psd::point<double>& scale() const;
	const double angle() const;
	const double shear() const;

	bool is_text() const;
	const boost::property_tree::wptree& text_data() const;

	bool is_solid() const;
	color<std::uint8_t> solid_color() const;

	bool has_timeline() const;
	const boost::property_tree::wptree& timeline_data() const;

	const point<int>& location() const;
	const psd::size<int>& size() const;
	const image8bit_ptr& bitmap() const;

	layer_type group_mode() const;

	int link_group_id() const;
	void set_link_group_id(int id);

	bool is_explicit_dynamic() const;
	bool is_static() const;
	bool is_movable() const;
	bool is_resizable() const;
	bool is_placeholder() const;
	bool is_cornerpin() const;
	layer_tag tags() const;
};

ENUM_ENABLE_BITWISE(layer::vector_mask_info::flags);

}	//namespace psd
}	//namespace caspar
