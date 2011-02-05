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

#include "..\..\frame\FramePlaybackStrategy.h"

namespace caspar { namespace bluefish {

class BlueFishVideoConsumer;

class BluefishPlaybackStrategy : public IFramePlaybackStrategy
{
	struct Implementation;
	std::shared_ptr<Implementation> pImpl_;

public:
	explicit BluefishPlaybackStrategy(BlueFishVideoConsumer* pConsumer, unsigned int memFmt, unsigned int updFmt, unsigned vidFmt);

	virtual void DisplayFrame(FramePtr);
	virtual IVideoConsumer* GetConsumer();
	virtual FrameManagerPtr GetFrameManager();
	virtual FramePtr GetReservedFrame();
};

}}
