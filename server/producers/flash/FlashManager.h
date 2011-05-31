#ifndef _FLASHMEDIAMANAGER_H__
#define _FLASHMEDIAMANAGER_H__

#include "..\..\MediaManager.h"

namespace caspar {

class FlashManager : public IMediaManager
{
public:
	FlashManager();
	virtual ~FlashManager();

	virtual MediaProducerPtr CreateProducer(const tstring& filename);
	virtual bool getFileInfo(FileInfo* pFileInfo);
};




}	//namespace caspar

#endif // _FLASHMEDIAMANAGER_H__
