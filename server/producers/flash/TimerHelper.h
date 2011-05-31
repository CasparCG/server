#ifndef _TIMER_HELPER_H__
#define _TIMER_HELPER_H__

#include "FlashAxContainer.h"

namespace caspar {
namespace flash {

	class TimerHelper
	{
		TimerHelper(const TimerHelper&);
		const TimerHelper& operator=(const TimerHelper&);

	public:
		TimerHelper()
		{}
		TimerHelper(DWORD first, DWORD interv, ITimerSink* pTS) : firstTime(first), interval(interv), currentTime(first), pTimerSink(pTS)
		{
			ID = first;
		}
		~TimerHelper()
		{
		}
		void Setup(DWORD first, DWORD interv, ITimerSink* pTS)
		{
			firstTime = first;
			interval = interv;
			currentTime = first;
			pTimerSink = pTS;
			ID = first;
		}

		void Invoke()
		{
			if(pTimerSink != 0)
			{
				VARIANT value;
				value.vt = VT_UI4;
				value.ulVal = currentTime;

				pTimerSink->OnTimer(value);
				currentTime += interval;
			}
		}

		DWORD firstTime;
		DWORD interval;
		DWORD currentTime;
		ATL::CComPtr<ITimerSink> pTimerSink;
		DWORD ID;
	};

}	//namespace flash
}	//namespace caspar

#endif	//_TIMER_HELPER_H__