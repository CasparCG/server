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

#ifndef	_PSDDOC_H__
#define _PSDDOC_H__

#pragma once

#include <string>
#include <vector>
#include "util\bigendian_file_input_stream.h"

#include "misc.h"
#include "layer.h"

namespace caspar { namespace psd {

class psd_document
{
public:
	psd_document();

	std::vector<layer_ptr>& layers()
	{
		return layers_;
	}
	unsigned long width() const
	{
		return width_;
	}
	unsigned long height() const
	{
		return height_;
	}

	psd::color_mode color_mode() const
	{
		return color_mode_;
	}

	unsigned short color_depth() const
	{
		return depth_;
	}
	unsigned short channels_count() const
	{
		return channels_;
	}
	const std::wstring& filename() const
	{
		return filename_;
	}
	bool has_timeline() const
	{
		return !timeline_desc_.empty();
	}
	const boost::property_tree::wptree& timeline() const 
	{
		return timeline_desc_;
	}


	bool parse(const std::wstring& s);

private:
	void read_header();
	void read_color_mode();
	void read_image_resources();
	void read_layers();

	std::wstring					filename_;
	BEFileInputStream				input_;

	std::vector<layer_ptr>			layers_;

	unsigned short					channels_;
	unsigned long					width_;
	unsigned long					height_;
	unsigned short					depth_;
	psd::color_mode					color_mode_;
	boost::property_tree::wptree	timeline_desc_;
};

}	//namespace psd
}	//namespace caspar

#endif	//_PSDDOC_H__