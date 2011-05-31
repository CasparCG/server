#ifndef _CASPAR_BLUEFISHVIDEOCONSUMER_H__
#define _CASPAR_BLUEFISHVIDEOCONSUMER_H__

#pragma once

#include "..\..\utils\thread.h"
#include "BluefishException.h"
#include "BluefishFrameManager.h"
#include "..\..\VideoConsumer.h"

#define TIMEOUT			1000

class CBlueVelvet;

namespace caspar {

class FramePlaybackControl;
typedef std::tr1::shared_ptr<FramePlaybackControl> FramePlaybackControlPtr;

namespace bluefish {

typedef std::tr1::shared_ptr<CBlueVelvet> BlueVelvetPtr;

class BlueFishVideoConsumer : public IVideoConsumer
{
	friend class BluefishPlaybackStrategy;

 	BlueFishVideoConsumer();
	BlueFishVideoConsumer(const BlueFishVideoConsumer&);
	const BlueFishVideoConsumer& operator=(const BlueFishVideoConsumer&);

public:
	virtual ~BlueFishVideoConsumer();

	static int EnumerateDevices();
	static VideoConsumerPtr Create(unsigned int deviceIndex);

	virtual IPlaybackControl* GetPlaybackControl() const;

	void EnableVideoOutput();
	void DisableVideoOutput();
	bool SetupDevice(unsigned int deviceIndex);
	bool ReleaseDevice();
	const TCHAR* GetFormatDescription() const {
		return FrameFormatDescription::FormatDescriptions[currentFormat_].name;
	}

private:
	BlueVelvetPtr pSDK_;
	FramePlaybackControlPtr pPlaybackControl_;
	BluefishFrameManagerPtr pFrameManager_;
	unsigned long m_bufferCount;
	unsigned long m_length;
	unsigned long m_actual;
	unsigned long m_golden;

	unsigned long GetVideoFormat(const tstring& strVideoMode);
	unsigned long VidFmtFromFrameFormat(FrameFormat fmt);

	FrameFormat currentFormat_;
	unsigned int _deviceIndex;
};
typedef std::tr1::shared_ptr<BlueFishVideoConsumer> BlueFishVideoConsumerPtr;

}	//namespace bluefish
}	//namespace caspar


#endif //_CASPAR_BLUEFISHVIDEOCONSUMER_H__
