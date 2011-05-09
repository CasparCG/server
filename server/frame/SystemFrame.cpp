/*
* copyright (c) 2010 Sveriges Television AB <info@casparcg.com>
*
*  This file is part of CasparCG.
*
*    CasparCG is free software: you can redistribute it and/or modify
*    it under the terms of the GNU General Public License as published by
*    the Free Software Foundation, either version 3 of the License, or
*    (at your option) any later version.
*
*    CasparCG is distributed in the hope that it will be useful,
*    but WITHOUT ANY WARRANTY; without even the implied warranty of
*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*    GNU General Public License for more details.

*    You should have received a copy of the GNU General Public License
*    along with CasparCG.  If not, see <http://www.gnu.org/licenses/>.
*
*/
 
#include "..\stdafx.h"
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

