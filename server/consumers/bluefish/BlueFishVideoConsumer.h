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
 
#ifndef _CASPAR_BLUEFISHVIDEOCONSUMER_H__
#define _CASPAR_BLUEFISHVIDEOCONSUMER_H__

#pragma once

#include "..\..\utils\thread.h"
#include "BluefishException.h"
#include "BluefishUtil.h"
#include "..\..\VideoConsumer.h"

#define TIMEOUT			1000

class CBlueVelvet4;

namespace caspar {

class FramePlaybackControl;
typedef std::tr1::shared_ptr<FramePlaybackControl> FramePlaybackControlPtr;

namespace bluefish {

typedef std::tr1::shared_ptr<CBlueVelvet4> BlueVelvetPtr;

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
	const TCHAR* GetFormatDescription() const 
	{
		return FrameFormatDescription::FormatDescriptions[currentFormat_].name;
	}
	bool SetVideoFormat(const tstring& strDesiredFrameFormat);

private:
	
	unsigned long BlueSetVideoFormat(tstring strDesiredFrameFormat);

	bool DoSetupDevice(unsigned int deviceIndex, tstring strDesiredFrameFormat);

	BlueVelvetPtr pSDK_;
	FramePlaybackControlPtr pPlaybackControl_;
	FrameManagerPtr pFrameManager_;
	unsigned long m_bufferCount;
	unsigned long m_length;
	unsigned long m_actual;
	unsigned long m_golden;
	
	FrameFormat currentFormat_;
	unsigned int _deviceIndex;
	bool hasEmbeddedAudio_;
		
	unsigned long memFmt_;
	unsigned long updFmt_;
	unsigned long vidFmt_; 
	unsigned long resFmt_; 
	unsigned long engineMode_;
};
typedef std::tr1::shared_ptr<BlueFishVideoConsumer> BlueFishVideoConsumerPtr;

}	//namespace bluefish
}	//namespace caspar


#endif //_CASPAR_BLUEFISHVIDEOCONSUMER_H__
