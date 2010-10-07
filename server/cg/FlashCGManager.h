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