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

#include "..\utils\event.h"
#include "..\utils\semaphore.h"
#include "..\utils\lockable.h"
#include "..\utils\ID.h"
#include "..\utils\Noncopyable.hpp"

#include <list>
#include <vector>

namespace caspar {

	namespace audio {
		class AudioDataChunk;
		typedef std::tr1::shared_ptr<AudioDataChunk> AudioDataChunkPtr;
	}

enum VideoUpdateMode {
	Interlaced = 0,
	Progressive
};

enum FrameFormat {
	FFormatPAL = 0,
	FFormatNTSC,
	FFormat576p2500,
	FFormat720p5000,
	FFormat720p5994,
	FFormat720p6000,
	FFormat1080p2397,
	FFormat1080p2400,
	FFormat1080i5000,
	FFormat1080i5994,
	FFormat1080i6000,
	FFormat1080p2500,
	FFormat1080p2997,
	FFormat1080p3000,
	FrameFormatCount,
	FFormatInvalid
};

struct FrameFormatDescription
{
	int width;
	int height;
	VideoUpdateMode mode;
	double fps;
	unsigned int size;
	const TCHAR* name;

	static const FrameFormatDescription FormatDescriptions[FrameFormatCount];
};

FrameFormat GetVideoFormat(const tstring& strVideoMode);

class Frame;
typedef std::tr1::shared_ptr<Frame> FramePtr;

class FrameManager;
typedef std::tr1::shared_ptr<FrameManager> FrameManagerPtr;

///////////////
// Frame
typedef unsigned int* FrameMetadata;
typedef std::vector<caspar::audio::AudioDataChunkPtr> AudioDataChunkList;

/*
	Frame

	Changes:
	2009-06-08 (R.N) : Added "FactoryID" which returns hashcode for the factory (framemanager) which own it
	2009-06-08 (R.N) : Added "Identifiable" which is used by consumer to see if the same frame is sent twice
*/

class Frame : public utils::Identifiable, private utils::Noncopyable
{
protected:
	Frame();
	void HasVideo(bool bHasVideo) const {
		bHasVideo_ = bHasVideo;
	}

public:
	virtual ~Frame();

	virtual unsigned char* GetDataPtr() const = 0;
	virtual bool HasValidDataPtr() const = 0;
	virtual unsigned int GetDataSize() const = 0;
	virtual FrameMetadata GetMetadata() const {
		return 0;
	}

	void AddAudioDataChunk(caspar::audio::AudioDataChunkPtr pChunk) {
		audioData_.push_back(pChunk);
	}
	AudioDataChunkList& GetAudioData() {
		return audioData_;
	}

	bool HasVideo() const {
		return bHasVideo_;
	}

	virtual const utils::ID& FactoryID() const = 0;

private:	
	mutable bool bHasVideo_;
	AudioDataChunkList audioData_;
};

///////////////////////////
// FrameBufferFetchResult
enum FrameBufferFetchResult {
	FetchDataAvailible,
	FetchWait,
	FetchEOF
};

/////////////////////
// FrameBuffer
class FrameBuffer
{
public:
	virtual ~FrameBuffer() 
	{}

	virtual FramePtr front() const = 0;
	virtual FrameBufferFetchResult pop_front() = 0;

	virtual void push_back(FramePtr) = 0;

	virtual void clear() = 0;

	virtual HANDLE GetWaitHandle() const = 0;
};

///////////////////////////
// StaticFrameBuffer
class StaticFrameBuffer : public FrameBuffer, private utils::Noncopyable
{	
public:
	StaticFrameBuffer();
	virtual ~StaticFrameBuffer();

	virtual FramePtr front() const;
	virtual FrameBufferFetchResult pop_front();
	virtual void push_back(FramePtr);
	virtual void clear();
	virtual HANDLE GetWaitHandle() const;

private:
	utils::Event event_;
	FramePtr pFrame_;
};

///////////////////////////
// MotionFrameBuffer
class MotionFrameBuffer : public FrameBuffer, private utils::LockableObject, private utils::Noncopyable
{	
public:
	MotionFrameBuffer();
	explicit MotionFrameBuffer(unsigned int);
	virtual ~MotionFrameBuffer();

	HANDLE GetWriteWaitHandle();

	virtual FramePtr front() const;
	virtual FrameBufferFetchResult pop_front();
	virtual void push_back(FramePtr);
	virtual void clear();
	virtual HANDLE GetWaitHandle() const;

private:
	unsigned int maxLength_;
	utils::Event event_;
	std::list<FramePtr> frameQueue_;
	utils::Event writeWaitEvent_; 
};

}	//namespace caspar