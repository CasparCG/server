#ifndef _SYSTEMFRAME_H_
#define _SYSTEMFRAME_H_

#include "Frame.h"

namespace caspar {

class SystemFrame : public Frame
{
public:

	SystemFrame(unsigned char* pData, unsigned int dataSize, const utils::ID& factoryID);
	virtual ~SystemFrame();

	virtual unsigned char* GetDataPtr() const;
	virtual bool HasValidDataPtr() const;
	virtual unsigned int GetDataSize() const;

	const utils::ID& FactoryID() const;

private:
	unsigned char* pData_;
	unsigned int dataSize_;
	utils::ID factoryID_;
};

}

#endif