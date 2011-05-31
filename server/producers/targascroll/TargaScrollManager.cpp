#include "..\..\StdAfx.h"

#include "TargaScrollManager.h"
#include "TargaScrollProducer.h"
#include "..\..\FileInfo.h"
#include "..\..\MediaManager.h"

namespace caspar {

TargaScrollMediaManager::TargaScrollMediaManager() : IMediaManager()
{
	IMediaManager::_extensions.push_back(TEXT("stga"));
}

TargaScrollMediaManager::~TargaScrollMediaManager()
{
}

MediaProducerPtr TargaScrollMediaManager::CreateProducer(const tstring& filename, FrameFormat format)
{
	TargaScrollMediaProducerPtr pTargaScrollMediaProducer(new TargaScrollMediaProducer(format));
	if (!pTargaScrollMediaProducer->Load(filename))
		pTargaScrollMediaProducer.reset();

	return pTargaScrollMediaProducer;
}

bool TargaScrollMediaManager::getFileInfo(FileInfo* pFileInfo)
{
	pFileInfo->length = 1;
	pFileInfo->type = TEXT("still");
	pFileInfo->encoding = TEXT("NA");

	return true;
}

}