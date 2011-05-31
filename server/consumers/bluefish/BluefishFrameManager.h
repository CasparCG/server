#ifndef _CASPAR_BLUEFISHVIDEOFRAMEFACTORY_H__
#define _CASPAR_BLUEFISHVIDEOFRAMEFACTORY_H__

#pragma once

#include "..\..\utils\Lockable.h"
#include "..\..\frame\FrameManager.h"
#include "..\..\frame\SystemFrameManager.h"
#include "..\..\frame\Frame.h"
#include <list>
#include <memory>
#include "BluefishException.h"

class CBlueVelvet;

namespace caspar {
namespace bluefish {

typedef std::tr1::shared_ptr<CBlueVelvet> BlueVelvetPtr;

class VideoFrameInfo
{
public:
	VideoFrameInfo() {}
	virtual ~VideoFrameInfo() {}

	virtual unsigned char* GetPtr() const = 0;
	virtual int GetBufferID() const = 0;
	virtual int GetDataSize() const = 0;
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
	int GetDataSize() const {
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
	int GetDataSize() const {
		return dataSize_;
	}

private:
	unsigned char* pData_;
	int bufferID_;
	int dataSize_;
};

class BluefishFrameManager : public FrameManager, private utils::LockableObject
{
	friend class BluefishVideoFrame;
	typedef std::list<VideoFrameInfoPtr> FrameInfoList;

	BluefishFrameManager(const BluefishFrameManager&);
	const BluefishFrameManager& operator=(const BluefishFrameManager&);

public:
	BluefishFrameManager(BlueVelvetPtr pSDK, FrameFormat fmt, unsigned long optimalLength);
	virtual ~BluefishFrameManager();

	virtual FramePtr CreateFrame();
	virtual const FrameFormatDescription& GetFrameFormatDescription() const;

private:
	VideoFrameInfoPtr GetBuffer();
	void ReturnBuffer(VideoFrameInfoPtr);

	BlueVelvetPtr pSDK_;
	FrameFormat format_;
	FrameInfoList frameBuffers_;
	SystemFrameManagerPtr	pBackupFrameManager_;
};
typedef std::tr1::shared_ptr<BluefishFrameManager> BluefishFrameManagerPtr;


class BluefishVideoFrame : public Frame
{
	friend class BluefishFrameManager;
	explicit BluefishVideoFrame(BluefishFrameManager* pFrameManager);

public:
	virtual ~BluefishVideoFrame();

	virtual unsigned char* GetDataPtr() const {
		if(pInfo_ != 0) {
			HasVideo(true);
			return pInfo_->GetPtr();
		}
		return 0;
	}
	virtual bool HasValidDataPtr() const {
		return (pInfo_ != 0 && pInfo_->GetPtr() != 0);
	}

	virtual unsigned int GetDataSize() const {
		return (pInfo_ != 0) ? pInfo_->GetDataSize() : 0;
	}
	virtual FrameMetadata GetMetadata() const {
		return (pInfo_ != 0) ? reinterpret_cast<FrameMetadata>(pInfo_->GetBufferID()) : 0;
	}
	const utils::ID& FactoryID() const {
		return factoryID_;
	}

private:
	VideoFrameInfoPtr pInfo_;
	BluefishFrameManager* pFrameManager_;
	utils::ID factoryID_;
};


}	//namespace bluefish
}	//namespace caspar

#endif	//_CASPAR_BLUEFISHVIDEOFRAMEFACTORY_H__