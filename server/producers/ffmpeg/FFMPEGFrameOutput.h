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
 
#ifndef _FFMPEG_FRAME_OUTPUT_H_
#define _FFMPEG_FRAME_OUTPUT_H_

#include "FFMPEGPacket.h"

#include "../../utils/Noncopyable.hpp"
#include "../../audio/audiomanager.h"

#include <memory>

namespace caspar
{
	namespace ffmpeg
	{

///=================================================================================================
/// <summary>	Merges video with audio and produces finished frames. </summary>
///
/// <remarks>	Rona 2010-06-19. </remarks>
///=================================================================================================
class FrameOutputFilter : public tbb::filter, private utils::Noncopyable
{
public:

	FrameOutputFilter();

	///=================================================================================================
	/// <summary>	Merges video with audio. </summary>
	///
	/// <remarks>	Rona 2010-06-19. </remarks>
	///
	/// <returns>	Finished frame. </returns>
	///=================================================================================================
	void* operator()(void* item);
				
private:
	struct Implementation;
	std::tr1::shared_ptr<Implementation> pImpl_;
};
typedef std::tr1::shared_ptr<FrameOutputFilter> FrameOutputFilterPtr;

	}
}

#endif
