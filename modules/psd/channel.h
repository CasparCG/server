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

#ifndef _PSDCHANNEL_H__
#define _PSDCHANNEL_H__

#pragma once

#include "util\bigendian_file_input_stream.h"
#include <memory>

namespace caspar { namespace psd {

class Channel
{
public:
	Channel(short id, unsigned long len) : id_(id), data_length_(len)
	{}

	short id() const
	{
		return id_;
	}
	unsigned long data_length() const
	{
		return data_length_;
	}

private:
	unsigned long data_length_;
	short id_;
};

typedef std::shared_ptr<Channel> channel_ptr;

}	//namespace psd
}	//namespace caspar

#endif	//_PSDCHANNEL_H__