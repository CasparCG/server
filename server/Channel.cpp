#include "stdAfx.h"

#include "Application.h"
#include "utils\critsectlock.h"
#include "Channel.h"

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
bool Channel::Stop()
{
	return pConsumer_->GetPlaybackControl()->StopPlayback();
}

bool Channel::Clear() 
{
	MediaProducerPtr pFP(GetApplication()->GetColorMediaManager()->CreateProducer(TEXT("#00000000")));
	return pConsumer_->GetPlaybackControl()->Load(pFP, false);
}

bool Channel::Param(const tstring& str) {
	return pConsumer_->GetPlaybackControl()->Param(str);
}

}