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

#include "../../../common/concurrency/thread.h"
#include "BluefishException.h"
#include "BluefishFrameManager.h"
#include "../../consumer/frame_consumer.h"
#include "../../renderer/render_device.h"
#include <tbb/concurrent_queue.h>
#include <boost/thread.hpp>

#define TIMEOUT			1000

class CBlueVelvet4;

namespace caspar {
	
namespace bluefish {

typedef std::tr1::shared_ptr<CBlueVelvet4> BlueVelvetPtr;

class BlueFishVideoConsumer : public frame_consumer
{
	friend class BluefishPlaybackStrategy;

 	BlueFishVideoConsumer(const frame_format_desc& format_desc);
	BlueFishVideoConsumer(const BlueFishVideoConsumer&);
	const BlueFishVideoConsumer& operator=(const BlueFishVideoConsumer&);

public:
	virtual ~BlueFishVideoConsumer();

	static int EnumerateDevices();
	static frame_consumer_ptr Create(const frame_format_desc& format_desc, unsigned int deviceIndex);
	
	void display(const frame_ptr&);
		
	const frame_format_desc& get_frame_format_desc() const { return format_desc_; }

private:

	void Run();

	void EnableVideoOutput();
	void DisableVideoOutput();
	bool SetupDevice(unsigned int deviceIndex);
	bool ReleaseDevice();

	bool DoSetupDevice(unsigned int deviceIndex);

	BlueVelvetPtr pSDK_;
	std::shared_ptr<BluefishPlaybackStrategy> pPlayback_;
	BluefishFrameManagerPtr pFrameManager_;
	unsigned long m_bufferCount;
	unsigned long m_length;
	unsigned long m_actual;
	unsigned long m_golden;

	unsigned long VidFmtFromFrameFormat(frame_format fmt);

	frame_format currentFormat_;
	unsigned int _deviceIndex;
	bool hasEmbeddedAudio_;
	frame_format_desc format_desc_;
	
	std::exception_ptr pException_;
	boost::thread thread_;
	tbb::concurrent_bounded_queue<frame_ptr> frameBuffer_;
};
typedef std::tr1::shared_ptr<BlueFishVideoConsumer> BlueFishFrameConsumerPtr;

}}
