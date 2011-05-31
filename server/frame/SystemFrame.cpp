#include "..\..\stdafx.h"
#include "SystemFrame.h"

namespace caspar {

	SystemFrame::SystemFrame(unsigned char* pData, unsigned int dataSize, const utils::ID& factoryID) : pData_(pData), dataSize_(dataSize), factoryID_(factoryID)
	{}

	SystemFrame::~SystemFrame()
	{}

	unsigned char* SystemFrame::GetDataPtr() const 
	{
		if(pData_ != 0)
			HasVideo(true);
		return pData_;
	}
	bool SystemFrame::HasValidDataPtr() const
	{
		return (pData_ != 0);
	}
	unsigned int SystemFrame::GetDataSize() const
	{
		return dataSize_;
	}

	const utils::ID& SystemFrame::FactoryID() const
	{
		return factoryID_;
	}
}

