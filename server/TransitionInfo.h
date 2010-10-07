/*
* copyright (c) 2010 Sveriges Television AB <info@casparcg.com>
*
*  This file is part of CasparCG.
*
*    CasparCG is free software: you can redistribute it and/or modify
*    it under the terms of the GNU General Public License as published by
*    the Free Software Foundation, either version 3 of the License, or
*    (at your option) any later version.
*
*    CasparCG is distributed in the hope that it will be useful,
*    but WITHOUT ANY WARRANTY; without even the implied warranty of
*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*    GNU General Public License for more details.

*    You should have received a copy of the GNU General Public License
*    along with CasparCG.  If not, see <http://www.gnu.org/licenses/>.
*
*/
 
#pragma once

#include <string>

namespace caspar {

enum TransitionType
{
	Cut = 1,
	Mix,
	Push,
	Slide,
	Wipe
};

enum TransitionDirection
{
	FromLeft = 1,
	FromRight,
	FromTop,
	FromBottom
};

class TransitionInfo
{
public:
	TransitionInfo() : type_(Cut), duration_(0), borderWidth_(0), borderColor_(TEXT("#00000000")), direction_(FromLeft)
	{}

	~TransitionInfo()
	{}

	TransitionType		type_;
	unsigned short		duration_;
	unsigned short		borderWidth_;
	tstring			borderImage_;
	tstring			borderColor_;
	TransitionDirection	direction_;
};

}	//namespace caspar