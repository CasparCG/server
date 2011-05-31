#pragma once

#include "..\..\frame\FramePlaybackStrategy.h"
#include <vector>

namespace caspar {
namespace bluefish {

class BlueFishVideoConsumer;

class BluefishPlaybackStrategy : public IFramePlaybackStrategy
{
public:
	explicit BluefishPlaybackStrategy(BlueFishVideoConsumer* pConsumer);
	virtual ~BluefishPlaybackStrategy();

	//IFramePlaybackStrategy
	virtual void DisplayFrame(Frame*);
	virtual IVideoConsumer* GetConsumer();
	virtual FrameManagerPtr GetFrameManager();
	virtual FramePtr GetReservedFrame();

private:
	void DoRender(Frame* pFrame);
	BlueFishVideoConsumer* pConsumer_;
	std::vector<FramePtr> reservedFrames_;
	int currentReservedFrameIndex_;
};

}	//namespace bluefish
}	//namespace caspar
