/*
* Copyright 2013 Sveriges Television AB http://casparcg.com/
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
* Author: Robert Nagy, ronag89@gmail.com
*/

#include "../stdafx.h"

#include "tbb_avcodec.h"

#include <common/log/log.h>
#include <common/env.h>
#include <common/utility/assert.h>

#include <tbb/task.h>
#include <tbb/atomic.h>
#include <tbb/parallel_for.h>
#include <tbb/tbb_thread.h>

#if defined(_MSC_VER)
#pragma warning (push)
#pragma warning (disable : 4244)
#endif
extern "C" 
{
	#define __STDC_CONSTANT_MACROS
	#define __STDC_LIMIT_MACROS
	#include <libavformat/avformat.h>
}
#if defined(_MSC_VER)
#pragma warning (pop)
#endif

namespace caspar {

int tbb_avcodec_open(AVCodecContext* avctx, AVCodec* codec)
{
	return avcodec_open2(avctx, codec, nullptr); 
}

int tbb_avcodec_close(AVCodecContext* avctx)
{
	return avcodec_close(avctx); 
}

}