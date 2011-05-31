#ifndef TARGASCROLLMEIDAMANAGER_H
#define TARGASCROLLMEIDAMANAGER_H

#include "..\..\MediaManager.h"

namespace caspar {

class TargaScrollMediaProducer;

class TargaScrollMediaManager : public IMediaManager
{
	public:
		TargaScrollMediaManager();
		virtual ~TargaScrollMediaManager();

		virtual bool getFileInfo(FileInfo* pFileInfo);
		virtual MediaProducerPtr CreateProducer(const tstring& filename, FrameFormat format);

	private:
		typedef std::tr1::shared_ptr<TargaScrollMediaProducer> TargaScrollMediaProducerPtr;
};

}

#endif