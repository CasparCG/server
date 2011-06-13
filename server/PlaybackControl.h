#pragma once

#include "MediaProducer.h"
#include "TransitionInfo.h"

namespace caspar
{
	namespace CG
	{
		class ICGControl;
	}

	class IPlaybackControl
	{
		IPlaybackControl(IPlaybackControl&);
		const IPlaybackControl& operator=(const IPlaybackControl&);

	public:
		IPlaybackControl() {}
		virtual ~IPlaybackControl() {}

		virtual bool Load(MediaProducerPtr pFP, bool loop) = 0;
		virtual bool LoadBackground(MediaProducerPtr pFP, const TransitionInfo& transitionInfo, bool loop) = 0;
		virtual bool Play() = 0;
		//virtual bool Pause() = 0;
		virtual bool StopPlayback(bool block = false) = 0;
		virtual bool IsRunning() = 0;
		virtual bool Param(const tstring& param) = 0;

		virtual CG::ICGControl* GetCGControl() = 0;
	};

	typedef std::tr1::shared_ptr<IPlaybackControl> PlaybackControlPtr;
}