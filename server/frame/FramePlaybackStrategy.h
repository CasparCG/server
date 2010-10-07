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
 
#ifndef _CASPAR_FRAMEPLAYBACKSTRATEGY_H__
#define _CASPAR_FRAMEPLAYBACKSTRATEGY_H__

#pragma once

#include "FrameManager.h"
#include "..\utils\Noncopyable.hpp"

namespace caspar {

class Frame;
class IVideoConsumer;

class IFramePlaybackStrategy : utils::Noncopyable
{
public:
	virtual ~IFramePlaybackStrategy() {}

	virtual FrameManagerPtr GetFrameManager() = 0;
	virtual void DisplayFrame(Frame*) = 0;
	virtual FramePtr GetReservedFrame() = 0;
};
typedef std::tr1::shared_ptr<IFramePlaybackStrategy> FramePlaybackStrategyPtr;

}	//namespace caspar

#endif	//_CASPAR_FRAMEPLAYBACKSTRATEGY_H__