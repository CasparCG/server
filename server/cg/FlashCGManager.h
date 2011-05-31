#pragma once

#include "..\channel.h"
#include "..\MediaManager.h"
#include <string>
#include <list>

namespace caspar {
	class Channel;
	class FlashManager;
	class FlashProducer;
	typedef std::tr1::shared_ptr<FlashProducer> FlashProducerPtr;

namespace CG {

class FlashCGManager
{
public:
	explicit FlashCGManager(caspar::Channel*);
public:
	virtual ~FlashCGManager();

	void Add(int layer, const tstring& templateName, unsigned int mixInDuration, bool playOnLoad, const tstring& startFromLabel, const tstring& data);
	void Remove(int layer);
	void Clear();
	void Play(int layer);
	void Stop(int layer, unsigned int mixOutDuration);
	void Next(int layer);
	void Goto(int layer, const tstring& label);
	void Update(int layer, const tstring& data);
	void Invoke(int layer, const tstring& methodSpec);


private:
	void DisplayActive();
	FlashProducerPtr CreateNewProducer();

	caspar::Channel* pChannel_;

	caspar::FlashManager* pFlashManager_;
	FlashProducerPtr activeCGProducer_;
};

typedef std::tr1::shared_ptr<FlashCGManager> FlashCGManagerPtr;

}	//namespace CG
}	//namespace caspar