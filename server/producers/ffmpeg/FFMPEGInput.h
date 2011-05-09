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
 
#ifndef _FFMPEG_INPUT_H_
#define _FFMPEG_INPUT_H_

#include "FFMPEGPacket.h"

#include "../../frame/FrameManager.h"
#include "../../utils/Noncopyable.hpp"

#include <tbb/pipeline.h>

#include <memory>
#include <system_error>

namespace caspar
{
	namespace ffmpeg
	{
	
typedef std::tr1::shared_ptr<AVFormatContext> AVFormatContextPtr;

///=================================================================================================
/// <summary>	Reads and creates packets from file. </summary>
///
/// <remarks>	Rona 2010-06-19. </remarks>
///=================================================================================================
class InputFilter : public tbb::filter, private utils::Noncopyable
{
public:

	///=================================================================================================
	/// <summary>	Default constructor. </summary>
	///
	/// <remarks>	Rona 2010-06-19. </remarks>
	///=================================================================================================
	InputFilter();

	///=================================================================================================
	/// <summary>	Loads the given file. </summary>
	///
	/// <remarks>	Rona 2010-06-19. </remarks>
	///
	/// <param name="filename">	Filename of the file. </param>
	/// <param name="errc">		[in,out] The errc. </param>
	///=================================================================================================
	void Load(const std::string& filename, std::error_code& errc = std::error_code());

	///=================================================================================================
	/// <summary>	Sets the factory used for allocating frames. </summary>
	///
	/// <remarks>	Rona 2010-06-19. </remarks>
	///
	/// <param name="pFrameManger">	The frame manger. </param>
	///
	/// <throws>	Does not throw. </throws>
	///
	/// <returns>	true if it succeeds, false if it fails. </returns>
	///=================================================================================================
	void SetFactory(const FrameManagerPtr& pFrameManger);

	///=================================================================================================
	/// <summary>	Video codec context. </summary>
	///
	/// <returns>	. </returns>
	///=================================================================================================
	const std::shared_ptr<AVCodecContext>& VideoCodecContext() const;

	///=================================================================================================
	/// <summary>	Audio codec context. </summary>
	///
	/// <returns>	. </returns>
	///=================================================================================================
	const std::shared_ptr<AVCodecContext>& AudioCodecContext() const;

	///=================================================================================================
	/// <summary>	Reads a packet from file. </summary>
	///
	/// <remarks>	Rona 2010-06-19. </remarks>
	///
	/// <returns>	A FFMPEG packet. </returns>
	///=================================================================================================
	void* operator()(void*);

	///=================================================================================================
	/// <summary>	Sets wheter the video should loop once finished. </summary>
	///
	/// <remarks>	Rona 2010-06-19. </remarks>
	///
	/// <param name="value">	true to value. </param>
	///=================================================================================================
	void SetLoop(bool value);

	///=================================================================================================
	/// <summary>	Gets the frame manager. </summary>
	///
	/// <returns>	The frame manager. </returns>
	///=================================================================================================
	const FrameManagerPtr GetFrameManager() const;

	///=================================================================================================
	/// <summary>	Stops this object. </summary>
	///=================================================================================================
	void Stop();

private:
	struct Implementation;
	std::tr1::shared_ptr<Implementation> pImpl_;
};
typedef std::tr1::shared_ptr<InputFilter> InputFilterPtr;

	}
}

#endif
