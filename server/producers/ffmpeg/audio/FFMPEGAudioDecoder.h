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
 
#ifndef _FFMPEG_AUDIODECODER_H_
#define _FFMPEG_AUDIODECODER_H_

#include "../FFMPEGPacket.h"

#include "../../../utils/Noncopyable.hpp"

#include <tbb/pipeline.h>

#include <memory>

namespace caspar
{
	namespace ffmpeg
	{
///=================================================================================================
/// <summary>	Decodes audio packets. </summary>
///
/// <remarks>	Rona 2010-06-19. </remarks>
///=================================================================================================
class AudioPacketDecoderFilter : public tbb::filter, private utils::Noncopyable
{
public:

	///=================================================================================================
	/// <summary>	Default constructor. </summary>
	///
	/// <remarks>	Rona 2010-06-19. </remarks>
	///=================================================================================================
	AudioPacketDecoderFilter();

	///=================================================================================================
	/// <summary>	Decodes audio packet. </summary>
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
	
	/// <summary> The alignment </summary>
	/// <remarks> Four sec of 16 bit stereo 48kHz should be enough </remarks>
	static const int ALIGNMENT = 16 ;

	/// <summary> Size of the audio decomp buffer </summary>
	static const int AUDIO_DECOMP_BUFFER_SIZE = 4*48000*4+ALIGNMENT;
private:
	struct Implementation;
	std::tr1::shared_ptr<Implementation> pImpl_;
};
typedef std::tr1::shared_ptr<AudioPacketDecoderFilter> AudioPacketDecoderFilterPtr;

	}
}

#endif