#pragma once

#include <string>
#include <vector>
#include "fileinfo.h"

namespace caspar {

class FileInfo;

class MediaProducer;
typedef std::tr1::shared_ptr<MediaProducer> MediaProducerPtr;

class IMediaManager
{
public:

	IMediaManager()
	{}

	virtual ~IMediaManager()
	{}
	virtual MediaProducerPtr CreateProducer(const tstring& filename) = 0;

	unsigned short GetSupportedExtensions(const std::vector<tstring>*& extensions)
	{
		extensions = &_extensions;
		return (unsigned short)_extensions.size();
	}

	virtual bool getFileInfo(FileInfo* pFileInfo) = 0;
protected:
	std::vector<tstring>	_extensions;
};
typedef std::tr1::shared_ptr<IMediaManager> MediaManagerPtr;

}	//namespace caspar