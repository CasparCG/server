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

namespace caspar { namespace psd {

class Layer;
typedef std::shared_ptr<Layer> layer_ptr;

class Layer
{
public:
	Layer() : blendMode_(InvalidBlendMode), opacity_(255), baseClipping_(false), flags_(0), masks_(0)
	{}

	static std::shared_ptr<Layer> create(BEFileInputStream&);
	void read_channel_data(BEFileInputStream&);

	const std::wstring& name() const
	{
		return name_;
	}
	const rect<long>& rect() const
	{
		return rect_;
	}

	const image8bit_ptr& image() const { return image_; }
	const image8bit_ptr& mask() const { return mask_; }

private:
	channel_ptr get_channel(ChannelType);
	void read_mask_data(BEFileInputStream&);
	void ReadBlendingRanges(BEFileInputStream&);

	void read_raw_image_data(BEFileInputStream& stream, const channel_ptr& channel, image8bit_ptr target, unsigned char offset);
	void read_rle_image_data(BEFileInputStream& stream, const channel_ptr& channel, image8bit_ptr target, unsigned char offset);

	caspar::psd::rect<long>			rect_;
	std::vector<channel_ptr>		channels_;
	BlendMode						blendMode_;
	unsigned char					opacity_;
	bool							baseClipping_;
	unsigned char					flags_;
	std::wstring					name_;

	unsigned char					masks_;
	caspar::psd::rect<long>			mask_rect_;
	unsigned char					default_mask_value_;
	unsigned char					mask_flags_;

	image8bit_ptr					image_;
	image8bit_ptr					mask_;
};

}	//namespace psd
}	//namespace caspar

#endif	//_PSDLAYER_H__