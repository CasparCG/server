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