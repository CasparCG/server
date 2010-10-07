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
 
#include "stdAfx.h"

#include "Application.h"
#include "utils\critsectlock.h"
#include "Channel.h"
#include <algorithm>

using namespace std;

namespace caspar {

using namespace caspar::utils;

Channel::Channel(int index, VideoConsumerPtr pConsumer) : pConsumer_(pConsumer), index_(index)
#ifdef ENABLE_MONITOR
														, monitor_(index)
#endif
{
}

Channel::~Channel() {
	Destroy();
}

bool Channel::Initialize() {
#ifdef ENABLE_MONITOR
	pConsumer_->GetPlaybackControl()->SetMonitor(&monitor_);
#endif
	return true;
}

void Channel::Destroy() {
	pConsumer_.reset();
}

const TCHAR* Channel::GetFormatDescription() const 
{
	return pConsumer_->GetFormatDescription();
}

bool Channel::IsPlaybackRunning() const 
{
	return pConsumer_->GetPlaybackControl()->IsRunning();
}

CG::ICGControl* Channel::GetCGControl() {
	return pConsumer_->GetPlaybackControl()->GetCGControl();
}

////////////////
// LOAD
bool Channel::Load(MediaProducerPtr pFP, bool loop)
{
	return pConsumer_->GetPlaybackControl()->Load(pFP, loop);
}

////////////////
// LOADBG
bool Channel::LoadBackground(MediaProducerPtr pFP, const TransitionInfo& transitionInfo, bool loop)
{
	return pConsumer_->GetPlaybackControl()->LoadBackground(pFP, transitionInfo, loop);
}

////////////////
// PLAY
bool Channel::Play()
{
	return pConsumer_->GetPlaybackControl()->Play();
}

////////////////
// STOP
bool Channel::Stop(bool block)
{
	return pConsumer_->GetPlaybackControl()->StopPlayback(block);
}

bool Channel::SetVideoFormat(const tstring& strDesiredFrameFormat)
{
	tstring strDesiredFrameFormatUpper = strDesiredFrameFormat;
	tstring strFmtDescUpper = this->pConsumer_->GetFormatDescription();

	std::transform(strDesiredFrameFormatUpper.begin(), strDesiredFrameFormatUpper.end(), strDesiredFrameFormatUpper.begin(), toupper);
	std::transform(strFmtDescUpper.begin(), strFmtDescUpper.end(), strFmtDescUpper.begin(), toupper);

	if(strDesiredFrameFormatUpper == strFmtDescUpper)
		return true;

	bool stopped = this->Stop(true); 	
	bool formatSet = stopped && this->pConsumer_->SetVideoFormat(strDesiredFrameFormat);	
	bool cleared = formatSet && this->Clear();

	return stopped && formatSet && cleared;
}



bool Channel::Clear() 
{	
	pConsumer_->GetPlaybackControl()->LoadEmpty();
	return true;
}

bool Channel::Param(const tstring& str) {
	return pConsumer_->GetPlaybackControl()->Param(str);
}

}	//namespace caspar