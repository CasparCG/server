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
 
#ifndef _FFMPEG_VIDEODEINTERLACER_H_
#define _FFMPEG_VIDEODEINTERLACER_H_

#include "../../../utils/Noncopyable.hpp"

#include <tbb/pipeline.h>

#include <memory>

namespace caspar
{
	namespace ffmpeg
	{
///=================================================================================================
/// <summary>	Deinterlaces video frames. </summary>
///
/// <remarks>	Rona 2010-06-19. This filter must be placed before FFMPEGVideoScaler. </remarks>
///=================================================================================================
class VideoPacketDeinterlacerFilter : public tbb::filter, private utils::Noncopyable
{
public:

	///=================================================================================================
	/// <summary>	Default constructor. </summary>
	///
	/// <remarks>	Rona 2010-06-19. </remarks>
	///=================================================================================================
	VideoPacketDeinterlacerFilter();

	///=================================================================================================
	/// <summary>	Deinterlaces video frame. </summary>
	///
	/// <remarks>	Rona 2010-06-19. </remarks>
	///
	/// <returns>	The result of the operation. </returns>
	///=================================================================================================
	void* operator()(void* item);

	///=================================================================================================
	/// <summary>	Destroys packet. </summary>
	///
	/// <remarks>	Rona 2010-06-19. </remarks>
	///
	/// <param name="item">	[in,out] If non-null, the item. </param>
	///=================================================================================================
	void finalize(void* item);

private:
	struct Implementation;
	std::tr1::shared_ptr<Implementation> pImpl_;
};

	}
}

#endif