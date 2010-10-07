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
 
#include "..\StdAfx.h"
#include "Frame.h"
#include "..\utils\allocator.h"
#include "..\utils\ID.h"
#include "..\utils\image\Image.hpp"
#include "FrameManager.h"
#include <algorithm>

#include <intrin.h>
#pragma intrinsic(__movsd, __stosd)

#define DEFINE_VIDEOFORMATDESC(w, h, m, f, s) { (w), (h), (m), (f), (w)*(h)*4, s }

namespace caspar {

const FrameFormatDescription FrameFormatDescription::FormatDescriptions[FrameFormatCount] =  {	
	DEFINE_VIDEOFORMATDESC(720, 576, Interlaced, 50, TEXT("PAL")), 
	DEFINE_VIDEOFORMATDESC(720, 486, Interlaced, 60/1.001, TEXT("NTSC")), 
	DEFINE_VIDEOFORMATDESC(720, 576, Progressive, 25, TEXT("576p2500")),
	DEFINE_VIDEOFORMATDESC(1280, 720, Progressive, 50, TEXT("720p5000")), 
	DEFINE_VIDEOFORMATDESC(1280, 720, Progressive, 60/1.001, TEXT("720p5994")),
	DEFINE_VIDEOFORMATDESC(1280, 720, Progressive, 60, TEXT("720p6000")),
	DEFINE_VIDEOFORMATDESC(1920, 1080, Progressive, 24/1.001, TEXT("1080p2397")),
	DEFINE_VIDEOFORMATDESC(1920, 1080, Progressive, 24, TEXT("1080p2400")),
	DEFINE_VIDEOFORMATDESC(1920, 1080, Interlaced, 50, TEXT("1080i5000")),
	DEFINE_VIDEOFORMATDESC(1920, 1080, Interlaced, 60/1.001, TEXT("1080i5994")),
	DEFINE_VIDEOFORMATDESC(1920, 1080, Interlaced, 60, TEXT("1080i6000")),
	DEFINE_VIDEOFORMATDESC(1920, 1080, Progressive, 25, TEXT("1080p2500")),
	DEFINE_VIDEOFORMATDESC(1920, 1080, Progressive, 30/1.001, TEXT("1080p2997")),
	DEFINE_VIDEOFORMATDESC(1920, 1080, Progressive, 30, TEXT("1080p3000"))
};


FrameFormat GetVideoFormat(const tstring& strVideoMode)
{
	for(int index = 0; index < FrameFormatCount; ++index)
	{
		const FrameFormatDescription& fmtDesc = FrameFormatDescription::FormatDescriptions[index];

		tstring strVideoModeUpper = strVideoMode;
		tstring strFmtDescUpper = fmtDesc.name;

		std::transform(strVideoModeUpper.begin(), strVideoModeUpper.end(), strVideoModeUpper.begin(), toupper);
		std::transform(strFmtDescUpper.begin(), strFmtDescUpper.end(), strFmtDescUpper.begin(), toupper);

		if(strVideoModeUpper == strFmtDescUpper) {
			return (FrameFormat)index;			
		}
	}
	return FFormatInvalid;
}

///////////////
// Frame
Frame::Frame() : bHasVideo_(false)
{}

Frame::~Frame() {
}

///////////////////////////
// StaticFrameBuffer
StaticFrameBuffer::StaticFrameBuffer() : event_(TRUE, FALSE) {
}
StaticFrameBuffer::~StaticFrameBuffer() {
}

FramePtr StaticFrameBuffer::front() const {
	return pFrame_;
}

FrameBufferFetchResult StaticFrameBuffer::pop_front() {
	if(pFrame_ == 0)
		return FetchWait;
	else
		return FetchEOF;
}

void StaticFrameBuffer::push_back(FramePtr pFrame) {
	pFrame_ = pFrame;
	event_.Set();
}

void StaticFrameBuffer::clear() {
	event_.Reset();
	pFrame_.reset();
}

HANDLE StaticFrameBuffer::GetWaitHandle() const {
	return event_;
}

///////////////////////////
// MotionFrameBuffer
MotionFrameBuffer::MotionFrameBuffer() : event_(TRUE, FALSE), writeWaitEvent_(TRUE, TRUE), maxLength_(3) {
}
MotionFrameBuffer::MotionFrameBuffer(unsigned int maxQueueLength) : event_(TRUE, FALSE), writeWaitEvent_(TRUE, TRUE), maxLength_(maxQueueLength) {
}
MotionFrameBuffer::~MotionFrameBuffer() {
}

FramePtr MotionFrameBuffer::front() const {
	Lock lock(*this);

	FramePtr result;
	if(frameQueue_.size() != 0)
		result = frameQueue_.front();
	return result;
}

FrameBufferFetchResult MotionFrameBuffer::pop_front() {
	Lock lock(*this);

	if(frameQueue_.size() == 0) {
		event_.Reset();
		return FetchWait;
	}

	if(frameQueue_.front() != 0) {
		frameQueue_.pop_front();
		
		if(frameQueue_.size() < maxLength_)
			writeWaitEvent_.Set();

		if(frameQueue_.size() == 0) {
			event_.Reset();
			return FetchWait;
		}

		if(frameQueue_.front() == 0)
			return FetchEOF;

		return FetchDataAvailible;
	}
	return FetchEOF;
}

void MotionFrameBuffer::push_back(FramePtr pFrame) {
	Lock lock(*this);

	frameQueue_.push_back(pFrame);
	event_.Set();

	if(frameQueue_.size() >= maxLength_)
		writeWaitEvent_.Reset();
}

void MotionFrameBuffer::clear() {
	Lock lock(*this);

	frameQueue_.clear();
	writeWaitEvent_.Set();
	event_.Reset();
}

HANDLE MotionFrameBuffer::GetWaitHandle() const {
	return event_;
}

HANDLE MotionFrameBuffer::GetWriteWaitHandle() {
	return writeWaitEvent_;
}

}	//namespace caspar