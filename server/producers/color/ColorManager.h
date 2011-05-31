#pragma once

#include "..\..\MediaManager.h"
#include "..\..\MediaProducer.h"

#include <string>

namespace caspar {

class FileInfo;

class ColorManager : public IMediaManager
{
public:
	ColorManager() {}
	virtual ~ColorManager() {}

	virtual MediaProducerPtr CreateProducer(const tstring& parameter);
	virtual bool getFileInfo(FileInfo* pFileInfo);

	static bool GetPixelColorValueFromString(const tstring& parameter, unsigned long* outValue);
};

}	//namespace caspar
