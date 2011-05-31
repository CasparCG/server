#include "..\stdafx.h"
#include "DCWrapper.h"

namespace caspar
{

DCWrapper::DCWrapper(HWND hWnd) : hWnd_(hWnd), dc_(::GetDC(hWnd))
{}

DCWrapper::~DCWrapper()
{
	if(dc_ != 0)
		::ReleaseDC(hWnd_, dc_);
}


}