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
 
#ifndef _FFMPEG_OUTPUT_H_
#define _FFMPEG_OUTPUT_H_

#include "FFMPEGPacket.h"

#include "../../frame/buffers/FrameBuffer.h"
#include "../../utils/Noncopyable.hpp"

#include <tbb/tbb_thread.h>

#include <memory>
#include <functional>

namespace caspar
{
	namespace ffmpeg
	{

///=================================================================================================
/// <summary>	Pushes frames into framebuffer. </summary>
///
/// <remarks>	Rona 2010-06-19. </remarks>
///=================================================================================================
class OutputFilter : public tbb::filter, private utils::Noncopyable
{
public:

	///=================================================================================================
	/// <summary>	Default constructor. </summary>
	///
	/// <remarks>	Rona 2010-06-19. </remarks>
	///=================================================================================================
	OutputFilter();

	///=================================================================================================
	/// <summary>	Pushes frames into framebuffer. Is blocking. </summary>
	///
	/// <remarks>	Rona 2010-06-19. </remarks>
	///
	/// <returns>	null </returns>
	///=================================================================================================
	void* operator()(void* item);

	///=================================================================================================
	/// <summary>	Unblocks and flushes frame queue. </summary>
	///=================================================================================================
	void Stop();	

	///=================================================================================================
	/// <summary>	Sends deferred frames to framebuffer. </summary>
	///=================================================================================================
	void Flush();

	///=================================================================================================
	/// <summary>	Gets the frame buffer. </summary>
	///
	/// <returns>	The frame buffer. </returns>
	///=================================================================================================
	FrameBuffer& GetFrameBuffer();

	///=================================================================================================
	/// <summary>	Sets the framebuffer capacity. </summary>
	///
	/// <remarks>	Rona 2010-06-19. </remarks>
	///
	/// <param name="capacity">	The capacity. </param>
	///=================================================================================================
	void SetCapacity(size_t capacity);

	///=================================================================================================
	/// <summary>	Sets the master thread which is allowed to block. </summary>
	///
	/// <remarks>	Rona 2010-06-19. </remarks>
	///
	/// <param name="master">	The master thread. </param>
	///=================================================================================================
	void SetMaster(tbb::tbb_thread::id master);
			
private:
	struct Implementation;
	std::tr1::shared_ptr<Implementation> pImpl_;
};
typedef std::tr1::shared_ptr<OutputFilter> OutputFilterPtr;

	}
}

#endif
