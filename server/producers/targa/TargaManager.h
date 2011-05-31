#pragma once

#include "..\..\frame\Frame.h"
#include "..\..\MediaManager.h"
#include "..\..\MediaProducer.h"
#include "..\..\utils\InputStream.h"
#include "..\..\utils\PixmapData.h"

namespace caspar {

class FileInfo;

class TargaManager : public IMediaManager
{
public:
	TargaManager();
	virtual ~TargaManager();

	virtual MediaProducerPtr CreateProducer(const tstring& filename);
	virtual bool getFileInfo(FileInfo* pFileInfo);

	static utils::PixmapDataPtr CropPadToFrameFormat(utils::PixmapDataPtr pSource, const FrameFormatDescription& fmtDesc);
	static bool Load(utils::InputStreamPtr pTGA, utils::PixmapDataPtr& pResult);
};

}	//namespace caspar