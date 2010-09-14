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
 
#ifndef _CASPAR_BLUEFISHVIDEOFRAMEFACTORY_H__
#define _CASPAR_BLUEFISHVIDEOFRAMEFACTORY_H__

#pragma once

#include "..\..\frame\system_frame.h"
#include "..\..\frame\frame_format.h"
#include <list>
#include <memory>
#include "BluefishException.h"

#include <tbb\mutex.h>

class CBlueVelvet4;

namespace caspar {
namespace bluefish {

typedef std::tr1::shared_ptr<CBlueVelvet4> BlueVelvetPtr;

class VideoFrameInfo
{
public:
	VideoFrameInfo() {}
	virtual ~VideoFrameInfo() {}

	virtual unsigned char* GetPtr() const = 0;
	virtual int GetBufferID() const = 0;
	virtual int size() const = 0;
};
typedef std::tr1::shared_ptr<VideoFrameInfo> VideoFrameInfoPtr;

class CardFrameInfo : public VideoFrameInfo
{
public:
	CardFrameInfo(BlueVelvetPtr pSDK, int dataSize, int bufferID);
	~CardFrameInfo();

	unsigned char* GetPtr() const {
		return pData_;
	}
	int GetBufferID() const {
		return bufferID_;
	}
	int size() const {
		return dataSize_;
	}

private:
	BlueVelvetPtr pSDK_;
	unsigned char* pData_;
	int bufferID_;
	int dataSize_;
};

class SystemFrameInfo : public VideoFrameInfo
{
public:
	SystemFrameInfo(int dataSize, int bufferID);
	~SystemFrameInfo();

	unsigned char* GetPtr() const {
		return pData_;
	}
	int GetBufferID() const {
		return bufferID_;
	}
	int size() const {
		return dataSize_;
	}

private:
	unsigned char* pData_;
	int bufferID_;
	int dataSize_;
};

class BluefishFrameManager
{
	friend class BluefishVideoFrame;
	typedef std::list<VideoFrameInfoPtr> FrameInfoList;

	BluefishFrameManager(const BluefishFrameManager&);
	const BluefishFrameManager& operator=(const BluefishFrameManager&);

public:
	BluefishFrameManager(BlueVelvetPtr pSDK, frame_format fmt, unsigned long optimalLength);
	virtual ~BluefishFrameManager();

	virtual std::shared_ptr<BluefishVideoFrame> CreateFrame();
	virtual const frame_format_desc& get_frame_format_desc() const;

private:
	VideoFrameInfoPtr GetBuffer();
	void ReturnBuffer(VideoFrameInfoPtr);

	BlueVelvetPtr pSDK_;
	frame_format format_;
	FrameInfoList frameBuffers_;
	tbb::mutex mutex_;
};
typedef std::tr1::shared_ptr<BluefishFrameManager> BluefishFrameManagerPtr;


class BluefishVideoFrame : public frame
{
public:
	explicit BluefishVideoFrame(BluefishFrameManager* pFrameManager);

	virtual ~BluefishVideoFrame();

	unsigned char* data() { return data_;	}

	unsigned int size() const { return size_; }
	
	long meta_data() const { return buffer_id_; }

	void* tag() const
	{
		return pFrameManager_;
	}
private:
	unsigned char* data_;
	long buffer_id_;
	size_t size_;
	VideoFrameInfoPtr pInfo_;
	BluefishFrameManager* pFrameManager_;
};


}	//namespace bluefish
}	//namespace caspar

#endif	//_CASPAR_BLUEFISHVIDEOFRAMEFACTORY_H__