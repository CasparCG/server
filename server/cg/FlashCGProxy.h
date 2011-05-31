#ifndef _CASPAR_FLASHCGPROXY_H__
#define _CASPAR_FLASHCGPROXY_H__

#pragma once

#include "CGControl.h"

namespace caspar {

class FlashProducer;
typedef std::tr1::shared_ptr<FlashProducer> FlashProducerPtr;

typedef std::tr1::function<void()> EmptyCallback;

namespace CG { 

class FlashCGProxy;
typedef std::tr1::shared_ptr<FlashCGProxy> FlashCGProxyPtr;

class FlashCGProxy : public ICGControl
{
	FlashCGProxy();

public:
	~FlashCGProxy();

	static FlashCGProxyPtr Create();

	bool Initialize(FrameManagerPtr pFrameManager);
	FrameBuffer& GetFrameBuffer();
	bool IsEmpty() const;
	void SetEmptyAlert(EmptyCallback callback);
	void Stop();

	//ICGControl
	virtual void Add(int layer, const tstring& templateName, bool playOnLoad, const tstring& startFromLabel, const tstring& data);
	virtual void Remove(int layer);
	virtual void Clear();
	virtual void Play(int layer);
	virtual void Stop(int layer, unsigned int mixOutDuration);
	virtual void Next(int layer);
	virtual void Update(int layer, const tstring& data);
	virtual void Invoke(int layer, const tstring& label);
	
private:
	FlashProducerPtr pFlashProducer_;
};


}	//namespace CG
}	//namespace caspar

#endif	//_CASPAR_FLASHCGPROXY_H__