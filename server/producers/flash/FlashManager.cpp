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
 
#include "..\..\stdafx.h"

#include "FlashManager.h"
#include "..\..\frame\FrameManager.h"
#include "..\..\Application.h"
#include "..\..\utils\image\Image.hpp"
#include "TimerHelper.h"

#include <math.h>
#include <intrin.h>
#pragma intrinsic(__movsd, __stosd)

#include <objbase.h>
#include <guiddef.h>
#include <algorithm>

#include "FlashProducer.h"

namespace caspar {

using namespace utils;


FlashManager::FlashManager()
{
	_extensions.push_back(TEXT("swf"));

	//Check that flash is installed
	ATL::CComBSTR flashGUID(_T("{D27CDB6E-AE6D-11CF-96B8-444553540000}"));
	CLSID clsid;
	if(!SUCCEEDED(CLSIDFromString((LPOLESTR)flashGUID, &clsid))) {
		throw std::exception("No Flash activex player installed");
	}
}

FlashManager::~FlashManager() {
}

MediaProducerPtr FlashManager::CreateProducer(const tstring& filename)
{
	return FlashProducer::Create(filename);
}

bool FlashManager::getFileInfo(FileInfo* pFileInfo)
{
	if(pFileInfo->filetype == TEXT("swf"))
	{
		pFileInfo->length = 0;	//get real length from file
		pFileInfo->type = TEXT("movie");
		pFileInfo->encoding = TEXT("swf");
	}
	return true;
}


}	//namespace caspar