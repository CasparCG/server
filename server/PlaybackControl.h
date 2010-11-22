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

#include "MediaProducer.h"
#include "TransitionInfo.h"

namespace caspar
{
	namespace CG
	{
		class ICGControl;
	}

	class Monitor;

	class IPlaybackControl
	{
		IPlaybackControl(IPlaybackControl&);
		const IPlaybackControl& operator=(const IPlaybackControl&);

	public:
		IPlaybackControl() {}
		virtual ~IPlaybackControl() {}

		virtual void SetMonitor(Monitor* pMonitor) = 0;

		virtual bool Load(MediaProducerPtr pFP, bool loop) = 0;
		virtual bool LoadBackground(MediaProducerPtr pFP, const TransitionInfo& transitionInfo, bool loop) = 0;
		virtual bool Play() = 0;
		virtual void LoadEmpty() = 0;
		//virtual bool Pause() = 0;
		virtual bool StopPlayback(bool block = false) = 0;
		virtual bool IsRunning() = 0;
		virtual bool Param(const tstring& param) = 0;

		virtual CG::ICGControl* GetCGControl() = 0;
	};

	typedef std::tr1::shared_ptr<IPlaybackControl> PlaybackControlPtr;
}