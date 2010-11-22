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
 
#ifndef _CASPAR_FLASHCGPROXY_H__
#define _CASPAR_FLASHCGPROXY_H__

#pragma once

#include "CGControl.h"

namespace caspar {

class Monitor;

class FlashProducer;
typedef std::tr1::shared_ptr<FlashProducer> FlashProducerPtr;

typedef std::tr1::function<void()> EmptyCallback;

namespace CG { 

class FlashCGProxy;
typedef std::tr1::shared_ptr<FlashCGProxy> FlashCGProxyPtr;

class FlashCGProxy : public ICGControl
{
	static int cgVersion_;

protected:
	FlashCGProxy();

public:
	virtual ~FlashCGProxy();

	static FlashCGProxyPtr Create(Monitor* pMonitor = 0);

	static void SetCGVersion();

	bool Initialize(FrameManagerPtr pFrameManager);
	FrameBuffer& GetFrameBuffer();
	bool IsEmpty() const;
	void SetEmptyAlert(EmptyCallback callback);
	void Stop();

	//ICGControl
	virtual void Clear();
	virtual void Add(int layer, const tstring& templateName,  bool playOnLoad, const tstring& startFromLabel = TEXT(""), const tstring& data = TEXT("")) {}
	virtual void Remove(int layer) {}
	virtual void Play(int layer) {}
	virtual void Stop(int layer, unsigned int mixOutDuration) {}
	virtual void Next(int layer) {}
	virtual void Update(int layer, const tstring& data) {}
	virtual void Invoke(int layer, const tstring& label) {}
	
	FlashProducerPtr GetFlashProducer() {
		return pFlashProducer_;
	}
protected:
	FlashProducerPtr pFlashProducer_;
};


}	//namespace CG
}	//namespace caspar

#endif	//_CASPAR_FLASHCGPROXY_H__