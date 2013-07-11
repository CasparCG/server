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
#include "descriptor.h"
//#include <iostream>

typedef unsigned char uint8_t;
#include <algorithm>
#include "../image/util/image_algorithms.h"
#include "../image/util/image_view.h"

namespace caspar { namespace psd {

void read_raw_image_data(BEFileInputStream& stream, const channel_ptr& channel, image8bit_ptr target, unsigned char offset);
void read_rle_image_data(BEFileInputStream& stream, const channel_ptr& channel, image8bit_ptr target, unsigned char offset);

layer_ptr layer::create(BEFileInputStream& stream)
{
	layer_ptr result(std::make_shared<layer>());
	result->rect_.top = stream.read_long();
	result->rect_.left = stream.read_long();
	result->rect_.bottom = stream.read_long();
	result->rect_.right = stream.read_long();

	//Get info about the channels in the layer
	unsigned short channelCount = stream.read_short();
	for(int channelIndex = 0; channelIndex < channelCount; ++channelIndex)
	{
		short channel_id = static_cast<short>(stream.read_short());
		unsigned long data_length = stream.read_long();

		if(channel_id <= -2)
			result->masks_++;

		channel_ptr channel(std::make_shared<Channel>(channel_id, data_length));
		result->channels_.push_back(channel);
	}

	unsigned long blendModeSignature = stream.read_long();
	if(blendModeSignature != '8BIM')
		throw PSDFileFormatException();

	unsigned long blendModeKey = stream.read_long();
	result->blend_mode_ = int_to_blend_mode(blendModeKey);

	result->opacity_ = stream.read_byte();
	result->baseClipping_ = stream.read_byte() == 1 ? false : true;
	result->flags_ = stream.read_byte();

	stream.discard_bytes(1);

	unsigned long extraDataSize = stream.read_long();
	long position1 = stream.current_position();
	result->mask_.read_mask_data(stream);
	result->read_blending_ranges(stream);

	result->name_ = stream.read_pascal_string(4);

	//Aditional Layer Information
	unsigned long end_of_layer_info = position1 + extraDataSize;
	
	try
	{
		while(stream.current_position() < end_of_layer_info)
		{
			unsigned long signature = stream.read_long();
			if(signature != '8BIM' && signature != '8B64')
				throw PSDFileFormatException();

			unsigned long key = stream.read_long();
			unsigned long length = stream.read_long();
			unsigned long end_of_chunk = stream.current_position() + length;

			if(key == 'TySh')	//type tool object settings
			{
				std::wstring text;	//the text in the layer

				stream.read_short();	//should be 1
				stream.discard_bytes(6*8);	//just throw transformation info for now
				stream.read_short();	//"text version" should be 50
				stream.read_long();		//"descriptor version" should be 16

				//text data descriptor ('descriptor structure')
				descriptor text_descriptor;
				if(!text_descriptor.populate(stream))
					throw PSDFileFormatException();

				stream.read_short();	//"warp version" should be 1
				stream.read_long();		//"descriptor version" should be 16

				//warp data descriptor ('descriptor structure')
				descriptor warp_descriptor;
				if(!text_descriptor.populate(stream))
					throw PSDFileFormatException();

				stream.discard_bytes(4*8);	//top, left, right, bottom
			}

			stream.set_position(end_of_chunk);
		}
	}
	catch(PSDFileFormatException& ex)
	{
		stream.set_position(end_of_layer_info);
	}
	
	return result;
}

void layer::read_mask_data(BEFileInputStream& stream)
{
	unsigned long length = stream.read_long();
	switch(length)
	{
	case 0:
		break;

	case 20:
		mask_.rect_.top = stream.read_long();
		mask_.rect_.left = stream.read_long();
		mask_.rect_.bottom = stream.read_long();
		mask_.rect_.right = stream.read_long();

		mask_.default_value_ = stream.read_byte();
		mask_.flags_ = stream.read_byte();
		stream.discard_bytes(2);
		break;

	case 36:
		stream.discard_bytes(18);	//we don't care about the user mask if there is a "total user mask"
		mask_.flags_ = stream.read_byte();
		mask_.default_value_ = stream.read_byte();
		mask_.rect_.top = stream.read_long();
		mask_.rect_.left = stream.read_long();
		mask_.rect_.bottom = stream.read_long();
		mask_.rect_.right = stream.read_long();
		break;

	default:
		stream.discard_bytes(length);
		break;
	};
}

//TODO: implement
void layer::read_blending_ranges(BEFileInputStream& stream)
{
	unsigned long length = stream.read_long();
	stream.discard_bytes(length);
}

channel_ptr layer::get_channel(channel_type type)
{
	auto end = channels_.end();
	for(auto it = channels_.begin(); it != end; ++it)
	{
		if((*it)->id() == type)
		{
			return (*it);
		}
	}

	return NULL;
}

void layer::read_channel_data(BEFileInputStream& stream)
{
	image8bit_ptr img;
	image8bit_ptr mask;

	bool has_transparency(get_channel(psd::Transparency));

	//std::clog << std::endl << "layer: " << std::string(name().begin(), name().end()) << std::endl;
	
	if(rect_.width() > 0 && rect_.height() > 0)
	{
		img = std::make_shared<image8bit>(rect_.width(), rect_.height(), std::min<unsigned char>(channels_.size() - masks_, 4));
		//std::clog << std::dec << "has image: [width: " << rect_.width() << " height: " << rect_.height() << "]" << std::endl;

		if(!has_transparency)
			std::memset(img->data(), (unsigned long)(255<<24), rect_.width()*rect_.height());
	}

	if(masks_ > 0 && mask_.rect_.width() > 0 && mask_.rect_.height() > 0)
	{
		mask = std::make_shared<image8bit>(mask_.rect_.width(), mask_.rect_.height(), 1);
		//std::clog << std::dec << "has mask: [width: " << mask_rect_.width() << " height: " << mask_rect_.height() << "]" << std::endl;
	}

	auto end = channels_.end();
	for(auto it = channels_.begin(); it != end; ++it)
	{
		image8bit_ptr target;
		unsigned char offset;
		bool discard_channel = false;

		//determine target bitmap and offset
		if((*it)->id() >= 3)
			discard_channel = true;	//discard channels that doesn't contribute to the final image
		else if((*it)->id() >= -1)	//BGRA-data
		{
			target = img;
			offset = ((*it)->id() >= 0) ? 2 - (*it)->id() : 3;
		}
		else if(mask)	//mask
		{
			if((*it)->id() == -2 && masks_ == 2)	//if there are two mask-channels, discard the the one that's not the total mask
				discard_channel = true;
			else
			{
				target = mask;
				offset = 0;
			}
		}

		//unsigned long cp = stream.current_position();	//for debug purposes only
		//std::clog << std::dec << "channel_id: " << (*it)->id() << ", reading data from: " << std::hex << cp << ", data_length: " << (*it)->data_length() << std::endl;

		if(!target)
			discard_channel = true;

		if(discard_channel)
		{
			//std::clog << "	-> discarding" << std::endl;
			stream.discard_bytes((*it)->data_length());
		}
		else
		{
			//std::clog << "	-> reading...";
			unsigned short encoding = stream.read_short();
			if(target)
			{
				if(encoding == 0)
					read_raw_image_data(stream, *it, target, offset);
				else if(encoding == 1)
					read_rle_image_data(stream, *it, target, offset);
				else
					throw PSDFileFormatException();
			}
			//std::clog << " " << std::hex << (stream.current_position() - cp) << " bytes read" << std::endl;
		}
	}

	if(img && has_transparency)
	{
		caspar::image::image_view<caspar::image::bgra_pixel> view(img->data(), img->width(), img->height());
		caspar::image::premultiply(view);
	}

	image_ = img;
	mask_.mask_ = mask;
}

void read_raw_image_data(BEFileInputStream& stream, const channel_ptr& channel, image8bit_ptr target, unsigned char offset)
{
	unsigned long total_length = target->width() * target->height();
	if(total_length != (channel->data_length() - 2))
		throw PSDFileFormatException();

	unsigned char* data = target->data();

	unsigned char stride = target->channel_count();
	if(stride == 1)
		stream.read(reinterpret_cast<char*>(data + offset), total_length);
	else
	{
		for(unsigned long index=0; index < total_length; ++index)
			data[index*stride+offset] = stream.read_byte();
	}
}

void read_rle_image_data(BEFileInputStream& stream, const channel_ptr& channel, image8bit_ptr target, unsigned char offset)
{
	unsigned long width = target->width();
	unsigned char stride = target->channel_count();
	unsigned long height = target->height();

	std::vector<unsigned short> scanline_lengths;
	scanline_lengths.reserve(height);

	for(unsigned long scanlineIndex=0; scanlineIndex < height; ++scanlineIndex)
		scanline_lengths.push_back(stream.read_short());

	unsigned char* data = target->data();

	for(unsigned long scanlineIndex=0; scanlineIndex < height; ++scanlineIndex)
	{
		unsigned long colIndex = 0;
		unsigned char length = 0;
		do
		{
			length = 0;

			//Get controlbyte
			char controlByte = static_cast<char>(stream.read_byte());
			if(controlByte >= 0)
			{
				//Read uncompressed string
				length = controlByte+1;
				for(unsigned long index=0; index < length; ++index)
					data[(scanlineIndex*width+colIndex+index) * stride + offset] = stream.read_byte();
			}
			else if(controlByte > -128)
			{
				//Repeat next byte
				length = -controlByte+1;
				unsigned value = stream.read_byte();
				for(unsigned long index=0; index < length; ++index)
					data[(scanlineIndex*width+colIndex+index) * stride + offset] = value;
			}

			colIndex += length;
		}
		while(colIndex < width);
	}
}

}	//namespace psd
}	//namespace caspar