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

#include "layer.h"
#include "psd_document.h"
#include "descriptor.h"
#include "util/pdf_reader.h"

#include "../image/util/image_algorithms.h"
#include "../image/util/image_view.h"

#include <common/log.h>

#include <boost/property_tree/ptree.hpp>
#include <boost/lexical_cast.hpp>

#include <algorithm>

namespace caspar { namespace psd {

void layer::layer_mask_info::read_mask_data(bigendian_file_input_stream& stream)
{
	unsigned long length = stream.read_long();
	switch(length)
	{
	case 0:
		break;

	case 20:
	case 36:	//discard total user mask data
		rect_.location.y = stream.read_long();
		rect_.location.x = stream.read_long();
		rect_.size.height = stream.read_long() - rect_.location.y;
		rect_.size.width = stream.read_long() - rect_.location.x;

		default_value_ = stream.read_byte();
		flags_ = stream.read_byte();
		stream.discard_bytes(2);
		
		if(length == 36)
		{
			stream.discard_bytes(18);	//discard "total user mask" data
			//flags_ = stream.read_byte();
			//default_value_ = stream.read_byte();
			//rect_.location.y = stream.read_long();
			//rect_.location.x = stream.read_long();
			//rect_.size.height = stream.read_long() - rect_.location.y;
			//rect_.size.width = stream.read_long() - rect_.location.x;
		}
		break;

	default:
		//TODO: Log that we discard a mask that is not supported
		stream.discard_bytes(length);
		break;
	};
}

struct layer::impl
{
	friend class layer;

	impl() : blend_mode_(blend_mode::InvalidBlendMode), link_group_id_(0), opacity_(255), sheet_color_(0), baseClipping_(false), flags_(0), protection_flags_(0), masks_count_(0), text_scale_(1.0f)
	{}

private:
	std::vector<channel>			channels_;
	blend_mode						blend_mode_;
	int								link_group_id_;
	unsigned char					opacity_;
	unsigned short					sheet_color_;
	bool							baseClipping_;
	unsigned char					flags_;
	int								protection_flags_;
	std::wstring					name_;
	char							masks_count_;
	float							text_scale_;

	rect<long>						vector_mask_;
	layer::layer_mask_info			mask_;

	rect<long>						bitmap_rect_;
	image8bit_ptr					bitmap_;

	boost::property_tree::wptree	text_layer_info_;
	boost::property_tree::wptree	timeline_info_;

	color<unsigned char>			solid_color_;

public:
	void populate(bigendian_file_input_stream& stream, const psd_document& doc)
	{
		bitmap_rect_.location.y = stream.read_long();
		bitmap_rect_.location.x = stream.read_long();
		bitmap_rect_.size.height = stream.read_long() - bitmap_rect_.location.y;
		bitmap_rect_.size.width = stream.read_long() - bitmap_rect_.location.x;

		//Get info about the channels in the layer
		unsigned short channelCount = stream.read_short();
		for(int channelIndex = 0; channelIndex < channelCount; ++channelIndex)
		{
			short id = static_cast<short>(stream.read_short());
			channel c(id, stream.read_long());

			if(c.id < -1)
				masks_count_++;

			channels_.push_back(c);
		}

		unsigned long blendModeSignature = stream.read_long();
		if(blendModeSignature != '8BIM')
			throw psd_file_format_exception();

		blend_mode_ = int_to_blend_mode(stream.read_long());
		opacity_ = stream.read_byte();
		baseClipping_ = stream.read_byte() == 1 ? false : true;
		flags_ = stream.read_byte();

		stream.discard_bytes(1);	//padding

		unsigned long extras_size = stream.read_long();
		auto position = stream.current_position();
		mask_.read_mask_data(stream);
		read_blending_ranges(stream);
		name_ = stream.read_pascal_string(4);

		//Aditional Layer Information
		auto end_of_layer_info = position + extras_size;
		try
		{
			while(stream.current_position() < end_of_layer_info)
			{
				read_chunk(stream, doc);
			}
		}
		catch(psd_file_format_exception&)
		{
			stream.set_position(end_of_layer_info);
		}
	}

	void read_chunk(bigendian_file_input_stream& stream, const psd_document& doc, bool isMetadata = false)
	{
		auto signature = stream.read_long();
		if(signature != '8BIM' && signature != '8B64')
			throw psd_file_format_exception();

		auto key = stream.read_long();

		if(isMetadata) stream.read_long();

		auto length = stream.read_long();
		auto end_of_chunk = stream.current_position() + length;

		try
		{
			switch(key)
			{
			case 'SoCo':
				read_solid_color(stream);
				break;

			case 'lspf':	//protection settings
				protection_flags_ = stream.read_long();
				break;

			case 'Txt2':	//text engine data
				break;

			case 'luni':
				name_ = stream.read_unicode_string();
				break;

			case 'TySh':	//type tool object settings
				read_text_data(stream);
				break;
				
			case 'shmd':	//metadata
				read_metadata(stream, doc);
				break;

			case 'lclr':
				sheet_color_ = stream.read_short();
				break;
				
			case 'lyvr':	//layer version
				break;

			case 'lnkD':	//linked layer
			case 'lnk2':	//linked layer
			case 'lnk3':	//linked layer
				break;

			case 'vmsk':
				read_vector_mask(length, stream, doc.width(), doc.height());
				break;
				
			case 'tmln':
				read_timeline_data(stream);
				break;

			default:
				break;
			}
		}
		catch(psd_file_format_exception& ex)
		{
			//ignore failed chunks silently
			CASPAR_LOG(warning) << ex.what();
		}

		stream.set_position(end_of_chunk);
	}

	void read_solid_color(bigendian_file_input_stream& stream)
	{
		if(stream.read_long() != 16)	//"descriptor version" should be 16
			throw psd_file_format_exception();

		descriptor solid_descriptor;
		solid_descriptor.populate(stream);
		solid_color_.red = static_cast<unsigned char>(solid_descriptor.items().get(L"Clr .Rd  ", 0.0) + 0.5);
		solid_color_.green = static_cast<unsigned char>(solid_descriptor.items().get(L"Clr .Grn ", 0.0) + 0.5);
		solid_color_.blue = static_cast<unsigned char>(solid_descriptor.items().get(L"Clr .Bl  ", 0.0) + 0.5);
		solid_color_.alpha = 255;
	}

	void read_vector_mask(unsigned long length, bigendian_file_input_stream& stream, long doc_width, long doc_height)
	{
		typedef std::pair<unsigned long, unsigned long> path_point;

		stream.read_long(); // version
		stream.read_long(); // flags
		int path_records = (length-8) / 26;

		auto position = stream.current_position();

		std::vector<path_point> knots;
		for(int i=1; i <= path_records; ++i)
		{
			auto selector = stream.read_short();
			if(selector == 2)	//we only concern ourselves with closed paths 
			{
				auto p_y = stream.read_long();
				auto p_x = stream.read_long();
				path_point cp_prev(p_x, p_y);

				auto a_y = stream.read_long();
				auto a_x = stream.read_long();
				path_point anchor(a_x, a_y);

				auto n_y = stream.read_long();
				auto n_x = stream.read_long();
				path_point cp_next(n_x, n_y);

				if(anchor == cp_prev && anchor == cp_next)
					knots.push_back(anchor);
				else
				{	//we can't handle smooth curves yet
					CASPAR_THROW_EXCEPTION(psd_file_format_exception() << msg_info("we can't handle smooth curves yet"));
				}
			}

			stream.set_position(position + 26*i);
		}

		if(knots.size() != 4)	//we only support rectangular vector masks
			CASPAR_THROW_EXCEPTION(psd_file_format_exception() << msg_info("we only support rectangular vector masks"));

		//we only support rectangular vector masks
		if(!(knots[0].first == knots[3].first && knots[1].first == knots[2].first && knots[0].second == knots[1].second && knots[2].second == knots[3].second))
			CASPAR_THROW_EXCEPTION(psd_file_format_exception() << msg_info("we only support rectangular vector masks"));

		//the path_points are given in fixed-point 8.24 as a ratio with regards to the width/height of the document. we need to divide by 16777215.0f to get the real ratio.
		float x_ratio = doc_width / 16777215.0f;
		float y_ratio = doc_height / 16777215.0f;
		vector_mask_.location.x = static_cast<long>(knots[0].first * x_ratio +0.5f);								//add .5 to get propper rounding when converting to integer
		vector_mask_.location.y = static_cast<long>(knots[0].second * y_ratio +0.5f);								//add .5 to get propper rounding when converting to integer
		vector_mask_.size.width = static_cast<long>(knots[1].first * x_ratio +0.5f)	- vector_mask_.location.x;		//add .5 to get propper rounding when converting to integer
		vector_mask_.size.height = static_cast<long>(knots[2].second * y_ratio +0.5f) - vector_mask_.location.y;	//add .5 to get propper rounding when converting to integer
	}

	void read_metadata(bigendian_file_input_stream& stream, const psd_document& doc)
	{
		auto count = stream.read_long();
		for(unsigned long index = 0; index < count; ++index)
			read_chunk(stream, doc, true);
	}

	void read_timeline_data(bigendian_file_input_stream& stream)
	{
		if(stream.read_long() != 16)	//"descriptor version" should be 16
			CASPAR_THROW_EXCEPTION(psd_file_format_exception() << msg_info("descriptor version should be 16"));

		descriptor timeline_descriptor;
		timeline_descriptor.populate(stream);
		timeline_info_.swap(timeline_descriptor.items());
	}

	void read_text_data(bigendian_file_input_stream& stream)
	{
		std::wstring text;	//the text in the layer

		stream.read_short();	//should be 1
	
		//transformation info
		auto xx = stream.read_double();
		auto xy = stream.read_double();
		auto yx = stream.read_double();
		auto yy = stream.read_double();
		stream.read_double(); // tx
		stream.read_double(); // ty
		if(xx != yy || (xy != 0 && yx != 0))
			CASPAR_THROW_EXCEPTION(psd_file_format_exception() << msg_info("Rotation and non-uniform scaling of dynamic textfields is not supported yet"));

		text_scale_ = static_cast<float>(xx);

		if(stream.read_short() != 50)	//"text version" should be 50
			CASPAR_THROW_EXCEPTION(psd_file_format_exception() << msg_info("invalid text version"));

		if(stream.read_long() != 16)	//"descriptor version" should be 16
			CASPAR_THROW_EXCEPTION(psd_file_format_exception() << msg_info("Invalid descriptor version while reading text-data"));

		descriptor text_descriptor;
		text_descriptor.populate(stream);
		auto text_info = text_descriptor.items().get_optional<std::wstring>(L"EngineData");
		if(text_info.is_initialized())
		{
			std::string str(text_info.get().begin(), text_info.get().end());
			read_pdf(text_layer_info_, str);
		}

		if(stream.read_short() != 1)	//"warp version" should be 1
			CASPAR_THROW_EXCEPTION(psd_file_format_exception() << msg_info("invalid warp version"));

		if(stream.read_long() != 16)	//"descriptor version" should be 16
			CASPAR_THROW_EXCEPTION(psd_file_format_exception() << msg_info("Invalid descriptor version while reading text warp-data"));

		descriptor warp_descriptor;
		warp_descriptor.populate(stream);
		stream.read_double(); // w_top
		stream.read_double();  // w_left
		stream.read_double();  // w_right
		stream.read_double();  // w_bottom
	}

	//TODO: implement
	void read_blending_ranges(bigendian_file_input_stream& stream)
	{
		auto length = stream.read_long();
		stream.discard_bytes(length);
	}

	bool has_channel(channel_type type)
	{
		return std::find_if(channels_.begin(), channels_.end(), [=](const channel& c) { return c.id == static_cast<int>(type); }) != channels_.end();
	}

	void read_channel_data(bigendian_file_input_stream& stream)
	{
		image8bit_ptr bitmap;
		image8bit_ptr mask;

		bool has_transparency = has_channel(channel_type::transparency);
	
		rect<long> clip_rect;
		if(!bitmap_rect_.empty())
		{
			clip_rect = bitmap_rect_;

			if(!vector_mask_.empty())
				clip_rect = vector_mask_;

			bitmap = std::make_shared<image8bit>(clip_rect.size.width, clip_rect.size.height, 4);

			if(!has_transparency)
				std::memset(bitmap->data(), 255, bitmap->width()*bitmap->height()*bitmap->channel_count());
		}

		if(masks_count_ > 0 && !mask_.rect_.empty())
		{
			mask = std::make_shared<image8bit>(mask_.rect_.size.width, mask_.rect_.size.height, 1);
		}

		for(auto it = channels_.begin(); it != channels_.end(); ++it)
		{
			psd::rect<long> src_rect;
			image8bit_ptr target;
			unsigned char offset = 0;
			bool discard_channel = false;

			//determine target bitmap and offset
			if((*it).id >= 3)
				discard_channel = true;	//discard channels that doesn't contribute to the final image
			else if((*it).id >= -1)	//BGRA-data
			{
				target = bitmap;
				offset = static_cast<unsigned char>(((*it).id >= 0) ? 2 - (*it).id : 3);
				src_rect = bitmap_rect_;
			}
			else if(mask)	//mask
			{
				if((*it).id == -3)	//discard the "total user mask"
					discard_channel = true;
				else
				{
					target = mask;
					offset = 0;
					src_rect = mask_.rect_;
				}
			}

			if(!target || src_rect.empty())
				discard_channel = true;

			auto end_of_data = stream.current_position() + (*it).data_length;
			if(!discard_channel)
			{
				auto encoding = stream.read_short();
				if(target)
				{
					if(encoding == 0)
						read_raw_image_data(stream, (*it).data_length-2, target, offset);
					else if(encoding == 1)
						read_rle_image_data(stream, src_rect, (target == bitmap) ? clip_rect : mask_.rect_, target, offset);
					else
						CASPAR_THROW_EXCEPTION(psd_file_format_exception() << msg_info("Unhandled image data encoding: " + boost::lexical_cast<std::string>(encoding)));
				}
			}
			stream.set_position(end_of_data);
		}

		if(bitmap && has_transparency)
		{
			caspar::image::image_view<caspar::image::bgra_pixel> view(bitmap->data(), bitmap->width(), bitmap->height());
			caspar::image::premultiply(view);
		}

		bitmap_ = bitmap;
		bitmap_rect_ = clip_rect;
		mask_.bitmap_ = mask;
	}

	void read_raw_image_data(bigendian_file_input_stream& stream, unsigned long data_length, image8bit_ptr target, unsigned char offset)
	{
		unsigned long total_length = target->width() * target->height();
		if(total_length != data_length)
			CASPAR_THROW_EXCEPTION(psd_file_format_exception() << msg_info("total_length != data_length"));

		auto data = target->data();

		auto stride = target->channel_count();
		if(stride == 1)
			stream.read(reinterpret_cast<char*>(data + offset), total_length);
		else
		{
			for(unsigned long index=0; index < total_length; ++index)
				data[index*stride+offset] = stream.read_byte();
		}
	}

	void read_rle_image_data(bigendian_file_input_stream& stream, const rect<long>&src_rect, const rect<long>&clip_rect, image8bit_ptr target, unsigned char offset)
	{
		auto width = src_rect.size.width;
		auto height = src_rect.size.height;
		auto stride = target->channel_count();

		int offset_x = clip_rect.location.x - src_rect.location.x;
		int offset_y = clip_rect.location.y - src_rect.location.y;

		std::vector<unsigned short> scanline_lengths;
		scanline_lengths.reserve(height);

		for (long scanlineIndex = 0; scanlineIndex < height; ++scanlineIndex)
			scanline_lengths.push_back(stream.read_short());

		auto target_data = target->data();

		std::vector<unsigned char> line(width);

		for(long scanlineIndex=0; scanlineIndex < height; ++scanlineIndex)
		{
			if(scanlineIndex >= target->height()+offset_y)
				break;

			long colIndex = 0;

			do
			{
				unsigned char length = 0;

				//Get controlbyte
				char controlByte = static_cast<char>(stream.read_byte());
				if(controlByte >= 0)
				{
					//Read uncompressed string
					length = controlByte+1;
					for(unsigned long index=0; index < length; ++index)
						line[colIndex+index] = stream.read_byte();
				}
				else if(controlByte > -128)
				{
					//Repeat next byte
					length = -controlByte+1;
					unsigned char value = stream.read_byte();
					for(unsigned long index=0; index < length; ++index)
						line[colIndex+index] = value;
				}

				colIndex += length;
			}
			while(colIndex < width);

			//use line to populate target
			if(scanlineIndex >= offset_y)
				for(int index = offset_x; index < target->width(); ++index)
				{
					target_data[((scanlineIndex-offset_y)*target->width()+index-offset_x) * stride + offset] = line[index];
				}
		}
	}
};

layer::layer() : impl_(spl::make_shared<impl>()) {}

void layer::populate(bigendian_file_input_stream& stream, const psd_document& doc) { impl_->populate(stream, doc); }
void layer::read_channel_data(bigendian_file_input_stream& stream) { impl_->read_channel_data(stream); }

const std::wstring& layer::name() const { return impl_->name_; }
unsigned char layer::opacity() const { return impl_->opacity_; }
unsigned short layer::sheet_color() const { return impl_->sheet_color_; }

bool layer::is_visible() { return (impl_->flags_ & 2) == 0; }	//the (PSD file-format) documentation is is saying the opposite but what the heck
bool layer::is_position_protected() { return (impl_->protection_flags_& 4) == 4; }

float layer::text_scale() const { return impl_->text_scale_; }
bool layer::is_text() const { return !impl_->text_layer_info_.empty(); }
const boost::property_tree::wptree& layer::text_data() const { return impl_->text_layer_info_; }

bool layer::has_timeline() const { return !impl_->timeline_info_.empty(); }
const boost::property_tree::wptree& layer::timeline_data() const { return impl_->timeline_info_; }

bool layer::is_solid() const { return impl_->solid_color_.alpha != 0; }
color<unsigned char> layer::solid_color() const { return impl_->solid_color_; }

const point<long>& layer::location() const { return impl_->bitmap_rect_.location; }
const image8bit_ptr& layer::bitmap() const { return impl_->bitmap_; }

int layer::link_group_id() const { return impl_->link_group_id_; }
void layer::set_link_group_id(int id) { impl_->link_group_id_ = id; }

}	//namespace psd
}	//namespace caspar
