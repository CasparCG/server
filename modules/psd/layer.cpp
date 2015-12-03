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
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>

#include <algorithm>

namespace caspar { namespace psd {

void layer::mask_info::read_mask_data(bigendian_file_input_stream& stream)
{
	auto length = stream.read_long();
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
			//we save the information about the total mask in case the vector-mask has an unsupported shape
			total_mask_.reset(new layer::mask_info);
			total_mask_->flags_ = stream.read_byte();
			total_mask_->default_value_ = stream.read_byte();
			total_mask_->rect_.location.y = stream.read_long();
			total_mask_->rect_.location.x = stream.read_long();
			total_mask_->rect_.size.height = stream.read_long() - rect_.location.y;
			total_mask_->rect_.size.width = stream.read_long() - rect_.location.x;
		}
		break;

	default:
		//TODO: Log that we discard a mask that is not supported
		stream.discard_bytes(length);
		break;
	};
}

bool layer::vector_mask_info::populate(int length, bigendian_file_input_stream& stream, int doc_width, int doc_height)
{
	typedef std::pair<int, int> path_point;
	bool bFail = false;

	stream.read_long(); // version
	this->flags_ = static_cast<std::uint8_t>(stream.read_long()); // flags
	int path_records = (length - 8) / 26;

	auto position = stream.current_position();

	std::vector<path_point> knots;

	const int SELECTOR_SIZE = 2;
	const int PATH_POINT_SIZE = 4 + 4;
	const int PATH_POINT_RECORD_SIZE = SELECTOR_SIZE + (3 * PATH_POINT_SIZE);

	for (int i = 1; i <= path_records; ++i)
	{
		auto selector = stream.read_short();
		if (selector == 2)	//we only concern ourselves with closed paths 
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

			if (anchor == cp_prev && anchor == cp_next)
				knots.push_back(anchor);
			else
			{	//we can't handle smooth curves yet
				bFail = true;
			}
		}

		auto offset = PATH_POINT_RECORD_SIZE * i;
		stream.set_position(position + offset);
	}

	if ((knots.size() != 4) || (!(knots[0].first == knots[3].first && knots[1].first == knots[2].first && knots[0].second == knots[1].second && knots[2].second == knots[3].second)))
		bFail = true;

	if (bFail) {
		rect_.clear();
		flags_ = static_cast<std::uint8_t>(flags::unsupported);
	}
	else 
	{
		//the path_points are given in fixed-point 8.24 as a ratio with regards to the width/height of the document. we need to divide by 16777215.0f to get the real ratio.
		float x_ratio = doc_width / 16777215.0f;
		float y_ratio = doc_height / 16777215.0f;
		rect_.location.x = static_cast<int>(knots[0].first * x_ratio + 0.5f);								//add .5 to get propper rounding when converting to integer
		rect_.location.y = static_cast<int>(knots[0].second * y_ratio + 0.5f);								//add .5 to get propper rounding when converting to integer
		rect_.size.width = static_cast<int>(knots[1].first * x_ratio + 0.5f) - rect_.location.x;		//add .5 to get propper rounding when converting to integer
		rect_.size.height = static_cast<int>(knots[2].second * y_ratio + 0.5f) - rect_.location.y;	//add .5 to get propper rounding when converting to integer
	}

	return !bFail;
}

struct layer::impl
{
	friend class layer;

	impl() : blend_mode_(blend_mode::InvalidBlendMode), layer_type_(layer_type::content), link_group_id_(0), opacity_(255), sheet_color_(0), baseClipping_(false), flags_(0), protection_flags_(0), masks_count_(0), text_scale_(1.0f), tags_(layer_tag::none)
	{}

private:
	std::vector<channel>			channels_;
	blend_mode						blend_mode_;
	layer_type						layer_type_;
	int								link_group_id_;
	int								opacity_;
	int								sheet_color_;
	bool							baseClipping_;
	std::uint8_t					flags_;
	std::uint32_t					protection_flags_;
	std::wstring					name_;
	int								masks_count_;
	double							text_scale_;

	layer::mask_info				mask_;

	rect<int>						bitmap_rect_;
	image8bit_ptr					bitmap_;

	boost::property_tree::wptree	text_layer_info_;
	boost::property_tree::wptree	timeline_info_;

	color<std::uint8_t>				solid_color_;

	layer_tag						tags_;

public:
	void populate(bigendian_file_input_stream& stream, const psd_document& doc)
	{
		bitmap_rect_.location.y = stream.read_long();
		bitmap_rect_.location.x = stream.read_long();
		bitmap_rect_.size.height = stream.read_long() - bitmap_rect_.location.y;
		bitmap_rect_.size.width = stream.read_long() - bitmap_rect_.location.x;

		//Get info about the channels in the layer
		auto channelCount = stream.read_short();
		for(int channelIndex = 0; channelIndex < channelCount; ++channelIndex)
		{
			auto id = static_cast<std::int16_t>(stream.read_short());
			channel c(id, stream.read_long());

			if(c.id < -1)
				masks_count_++;

			channels_.push_back(c);
		}

		auto blendModeSignature = stream.read_long();
		if(blendModeSignature != '8BIM')
			CASPAR_THROW_EXCEPTION(psd_file_format_exception() << msg_info("blendModeSignature != '8BIM'"));

		blend_mode_ = int_to_blend_mode(stream.read_long());
		opacity_ = stream.read_byte();
		baseClipping_ = stream.read_byte() == 1 ? false : true;
		flags_ = stream.read_byte();

		stream.discard_bytes(1);	//padding

		auto extras_size = stream.read_long();
		auto position = stream.current_position();
		mask_.read_mask_data(stream);
		read_blending_ranges(stream);

		stream.read_pascal_string(4);	//throw this away. We'll read the unicode version of the name later

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
			CASPAR_THROW_EXCEPTION(psd_file_format_exception() << msg_info("signature != '8BIM' && signature != '8B64'"));

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

			case 'lsct':	//group settings (folders)
				read_group_settings(stream, length);

			case 'lspf':	//protection settings
				protection_flags_ = stream.read_long();
				break;

			case 'Txt2':	//text engine data
				break;

			case 'luni':
				set_name_and_tags(stream.read_unicode_string());
				break;

			case 'TySh':	//type tool object settings
				read_text_data(stream);
				break;
				
			case 'shmd':	//metadata
				read_metadata(stream, doc);
				break;

			case 'lclr':
				sheet_color_ = static_cast<std::int16_t>(stream.read_short());
				break;
				
			case 'lyvr':	//layer version
				break;

			case 'lnkD':	//linked layer
			case 'lnk2':	//linked layer
			case 'lnk3':	//linked layer
				break;

			case 'vmsk':
				mask_.read_vector_mask_data(length, stream, doc.width(), doc.height());
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
	  
	void set_name_and_tags(const std::wstring& name) {
		auto start_bracket = name.find_first_of(L'[');
		auto end_bracket = name.find_first_of(L']');
		if (start_bracket == std::wstring::npos && end_bracket == std::wstring::npos) {
			//no flags
			name_ = name;
		}
		else if (start_bracket != std::wstring::npos && end_bracket > start_bracket) {
			//we have tags
			tags_ = string_to_layer_tags(name.substr(start_bracket+1, end_bracket-start_bracket-1));
			name_ = name.substr(end_bracket+1);
		}
		else {
			//missmatch
			name_ = name;
			CASPAR_LOG(warning) << "Mismatching tag-brackets in layer name";
		}

		boost::trim(name_);
	}

	void read_solid_color(bigendian_file_input_stream& stream)
	{
		if(stream.read_long() != 16)	//"descriptor version" should be 16
			CASPAR_THROW_EXCEPTION(psd_file_format_exception() << msg_info("descriptor version should be 16"));

		descriptor solid_descriptor(L"solid_color");
		solid_descriptor.populate(stream);
		solid_color_.red = static_cast<std::uint8_t>(solid_descriptor.items().get(L"Clr .Rd  ", 0.0) + 0.5);
		solid_color_.green = static_cast<std::uint8_t>(solid_descriptor.items().get(L"Clr .Grn ", 0.0) + 0.5);
		solid_color_.blue = static_cast<std::uint8_t>(solid_descriptor.items().get(L"Clr .Bl  ", 0.0) + 0.5);
		solid_color_.alpha = 255;
	}

	void read_group_settings(bigendian_file_input_stream& stream, unsigned int length) 
	{
		auto type = stream.read_long();
		unsigned int sub_type = 0;

		if (length >= 12)
		{
			auto signature = stream.read_long();
			if (signature != '8BIM')
				CASPAR_THROW_EXCEPTION(psd_file_format_exception() << msg_info("signature != '8BIM'"));

			blend_mode_ = int_to_blend_mode(stream.read_long());
			
			if (length >= 16)
				sub_type = stream.read_long();
		}

		layer_type_ = int_to_layer_type(type, sub_type);
	}

	void read_metadata(bigendian_file_input_stream& stream, const psd_document& doc)
	{
		int count = stream.read_long();
		for(int index = 0; index < count; ++index)
			read_chunk(stream, doc, true);
	}

	void read_timeline_data(bigendian_file_input_stream& stream)
	{
		if(stream.read_long() != 16)	//"descriptor version" should be 16
			CASPAR_THROW_EXCEPTION(psd_file_format_exception() << msg_info("descriptor version should be 16"));

		descriptor timeline_descriptor(L"timeline");
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

		text_scale_ = xx;

		if(stream.read_short() != 50)	//"text version" should be 50
			CASPAR_THROW_EXCEPTION(psd_file_format_exception() << msg_info("invalid text version"));

		if(stream.read_long() != 16)	//"descriptor version" should be 16
			CASPAR_THROW_EXCEPTION(psd_file_format_exception() << msg_info("Invalid descriptor version while reading text-data"));

		descriptor text_descriptor(L"text");
		text_descriptor.populate(stream);
		auto text_info = text_descriptor.items().get_optional<std::wstring>(L"EngineData");
		
		if (text_info)
		{
			std::string str(text_info->begin(), text_info->end());
			read_pdf(text_layer_info_, str);
			log::print_child(boost::log::trivial::trace, L"", L"text_layer_info", text_layer_info_);
		}

		if(stream.read_short() != 1)	//"warp version" should be 1
			CASPAR_THROW_EXCEPTION(psd_file_format_exception() << msg_info("invalid warp version"));

		if(stream.read_long() != 16)	//"descriptor version" should be 16
			CASPAR_THROW_EXCEPTION(psd_file_format_exception() << msg_info("Invalid descriptor version while reading text warp-data"));

		descriptor warp_descriptor(L"warp");
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

		bool has_transparency = has_channel(channel_type::transparency);
	
		if(!bitmap_rect_.empty())
		{
			bitmap = std::make_shared<image8bit>(bitmap_rect_.size.width, bitmap_rect_.size.height, 4);
			if(!has_transparency)
				std::memset(bitmap->data(), 255, bitmap->width()*bitmap->height()*bitmap->channel_count());
		}

		for(auto it = channels_.begin(); it != channels_.end(); ++it)
		{
			auto channel = (*it);
			image8bit_ptr target;
			int offset = 0;
			bool discard_channel = false;

			//determine target bitmap and offset
			if(channel.id >= 3)
				discard_channel = true;	//discard channels that doesn't contribute to the final image
			else if(channel.id >= -1)	//BGRA-data
			{
				target = bitmap;
				offset = (channel.id >= 0) ? 2 - channel.id : 3;
			}
			else	//mask
			{
				offset = 0;
				if (channel.id == -2)
				{
					mask_.create_bitmap();
					target = mask_.bitmap_;
				}
				else if (channel.id == -3)	//total_mask
				{
					mask_.total_mask_->create_bitmap();
					target = mask_.total_mask_->bitmap_;
					offset = 0;
				}
			}

			if(!target)
				discard_channel = true;

			auto end_of_data = stream.current_position() + channel.data_length;
			if(!discard_channel)
			{
				auto encoding = stream.read_short();
				if(target)
				{
					if(encoding == 0)
						read_raw_image_data(stream, channel.data_length-2, target, offset);
					else if(encoding == 1)
						read_rle_image_data(stream, target, offset);
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
	}

	void read_raw_image_data(bigendian_file_input_stream& stream, int data_length, image8bit_ptr target, int offset)
	{
		auto total_length = target->width() * target->height();
		if (total_length != data_length)
			CASPAR_THROW_EXCEPTION(psd_file_format_exception() << msg_info("total_length != data_length"));

		auto data = target->data();
		auto stride = target->channel_count();

		if (stride == 1)
			stream.read(reinterpret_cast<char*>(data + offset), total_length);
		else
		{
			for(int index = 0; index < total_length; ++index)
				data[index * stride + offset] = stream.read_byte();
		}
	}

	void read_rle_image_data(bigendian_file_input_stream& stream, image8bit_ptr target, int offset)
	{
		auto width = target->width();
		auto height = target->height();
		auto stride = target->channel_count();

		std::vector<int> scanline_lengths;
		scanline_lengths.reserve(height);

		for (int scanlineIndex = 0; scanlineIndex < height; ++scanlineIndex)
			scanline_lengths.push_back(static_cast<std::int16_t>(stream.read_short()));

		auto target_data = target->data();

		std::vector<std::uint8_t> line(width);

		for(int scanlineIndex=0; scanlineIndex < height; ++scanlineIndex)
		{
			int colIndex = 0;

			do
			{
				int length = 0;

				//Get controlbyte
				char controlByte = static_cast<char>(stream.read_byte());
				if(controlByte >= 0)
				{
					//Read uncompressed string
					length = controlByte+1;
					for(int index=0; index < length; ++index)
						line[colIndex+index] = stream.read_byte();
				}
				else if(controlByte > -128)
				{
					//Repeat next byte
					length = -controlByte+1;
					auto value = stream.read_byte();
					for(int index=0; index < length; ++index)
						line[colIndex+index] = value;
				}

				colIndex += length;
			}
			while(colIndex < width);

			//use line to populate target
			for(int index = 0; index < width; ++index)
			{
				target_data[(scanlineIndex*width+index) * stride + offset] = line[index];
			}
		}
	}
};

layer::layer() : impl_(spl::make_shared<impl>()) {}

void layer::populate(bigendian_file_input_stream& stream, const psd_document& doc) { impl_->populate(stream, doc); }
void layer::read_channel_data(bigendian_file_input_stream& stream) { impl_->read_channel_data(stream); }

const std::wstring& layer::name() const { return impl_->name_; }
int layer::opacity() const { return impl_->opacity_; }
int layer::sheet_color() const { return impl_->sheet_color_; }

bool layer::is_visible() { return (impl_->flags_ & 2) == 0; }	//the (PSD file-format) documentation is is saying the opposite but what the heck
bool layer::is_position_protected() { return (impl_->protection_flags_& 4) == 4; }

const layer::mask_info& layer::mask() const { return impl_->mask_; }

double layer::text_scale() const { return impl_->text_scale_; }
bool layer::is_text() const { return !impl_->text_layer_info_.empty(); }
const boost::property_tree::wptree& layer::text_data() const { return impl_->text_layer_info_; }

bool layer::has_timeline() const { return !impl_->timeline_info_.empty(); }
const boost::property_tree::wptree& layer::timeline_data() const { return impl_->timeline_info_; }

bool layer::is_solid() const { return impl_->solid_color_.alpha != 0; }
color<std::uint8_t> layer::solid_color() const { return impl_->solid_color_; }

const point<int>& layer::location() const { return impl_->bitmap_rect_.location; }
const size<int>& layer::size() const { return impl_->bitmap_rect_.size; }
const image8bit_ptr& layer::bitmap() const { return impl_->bitmap_; }

layer_type layer::group_mode() const { return impl_->layer_type_; }
int layer::link_group_id() const { return impl_->link_group_id_; }
void layer::set_link_group_id(int id) { impl_->link_group_id_ = id; }

bool layer::is_explicit_dynamic() const { return (impl_->tags_ & layer_tag::explicit_dynamic) == layer_tag::explicit_dynamic; }
bool layer::is_static() const { return (impl_->tags_ & layer_tag::rasterized) == layer_tag::rasterized; }
bool layer::is_movable() const { return (impl_->tags_ & layer_tag::moveable) == layer_tag::moveable; }
bool layer::is_resizable() const { return (impl_->tags_ & layer_tag::resizable) == layer_tag::resizable; }
bool layer::is_placeholder() const { return (impl_->tags_ & layer_tag::placeholder) == layer_tag::placeholder; }
layer_tag layer::tags() const { return impl_->tags_; }

}	//namespace psd
}	//namespace caspar
