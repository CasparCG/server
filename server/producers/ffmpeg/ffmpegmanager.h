#pragma once

//std include
#include <vector>
#include <string>
#include <queue>

#include "..\..\MediaProducer.h"
#include "..\..\MediaManager.h"

//#include "..\..\utils\thread.h"

namespace caspar {
namespace ffmpeg {

class FFMPEGManager : public IMediaManager
{
public:
	FFMPEGManager();
	virtual ~FFMPEGManager();

	virtual MediaProducerPtr CreateProducer(const tstring& filename);
	virtual bool getFileInfo(FileInfo* pFileInfo);

	static const int Alignment;
	static const int AudioDecompBufferSize;
private:

	static long static_instanceCount;
	static bool Initialize();
	static void Dispose();
};

}	//namespace ffmpeg
}	//namespace caspar