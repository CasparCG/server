#include "stdAfx.h"

#include "Application.h"
#include "utils\critsectlock.h"
#include "Channel.h"

#include <algorithm>

using namespace std;

namespace caspar {

using namespace caspar::utils;

Channel::Channel(int index, VideoConsumerPtr pConsumer) : pConsumer_(pConsumer), index_(index)
{
}

Channel::~Channel() {
	Destroy();
}

bool Channel::Initialize() {
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

bool Channel::Clear() 
{
	MediaProducerPtr pFP(GetApplication()->GetColorMediaManager()->CreateProducer(TEXT("#00000000")));
	return pConsumer_->GetPlaybackControl()->Load(pFP, false);
}

bool Channel::Param(const tstring& str) {
	return pConsumer_->GetPlaybackControl()->Param(str);
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

}