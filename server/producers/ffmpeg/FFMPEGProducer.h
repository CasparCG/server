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
 
#ifndef _FFMPEG_PRODUCER_H_
#define _FFMPEG_PRODUCER_H_

#include "../../MediaProducer.h"
#include "../../MediaManager.h"
#include "../../MediaProducerInfo.h"

#include "../../utils/Noncopyable.hpp"

extern "C" 
{
	#define __STDC_CONSTANT_MACROS
	#define __STDC_LIMIT_MACROS
	#include <ffmpeg/avformat.h>
	#include <ffmpeg/avcodec.h>
	#include <ffmpeg/swscale.h>
	#include <ffmpeg/avutil.h>
//	#include <libavformat/avformat.h>
//	#include <libavcodec/avcodec.h>
//	#include <libswscale/swscale.h>
//	#include <libavutil/avutil.h>
}

#include <memory>

namespace caspar { namespace ffmpeg {

class FFMPEGProducer : public MediaProducer
{
public:
	FFMPEGProducer(const tstring& filename);
	
	IMediaController* QueryController(const tstring&);
	bool GetProducerInfo(MediaProducerInfo* pInfo);

	void SetLoop(bool loop);

	static const size_t MAX_TOKENS = 5;
	static const size_t MIN_BUFFER_SIZE = 2;
	static const size_t DEFAULT_BUFFER_SIZE = 16;
	static const size_t MAX_BUFFER_SIZE = 64;
	static const size_t LOAD_TARGET_BUFFER_SIZE = 8;
	static const size_t THREAD_TIMEOUT_MS = 1000;

private:	
	struct Implementation;
	std::shared_ptr<Implementation> pImpl_;
};
typedef std::tr1::shared_ptr<FFMPEGProducer> FFMPEGProducerPtr;

}}

#endif