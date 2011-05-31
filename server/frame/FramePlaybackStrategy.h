#ifndef _CASPAR_FRAMEPLAYBACKSTRATEGY_H__
#define _CASPAR_FRAMEPLAYBACKSTRATEGY_H__

#pragma once

#include "FrameManager.h"
#include "..\utils\Noncopyable.hpp"

namespace caspar {

class Frame;
class IVideoConsumer;

class IFramePlaybackStrategy : utils::Noncopyable
{
public:
	virtual ~IFramePlaybackStrategy() {}

	virtual FrameManagerPtr GetFrameManager() = 0;
	virtual void DisplayFrame(Frame*) = 0;
	virtual FramePtr GetReservedFrame() = 0;
};
typedef std::tr1::shared_ptr<IFramePlaybackStrategy> FramePlaybackStrategyPtr;

}	//namespace caspar

#endif	//_CASPAR_FRAMEPLAYBACKSTRATEGY_H__