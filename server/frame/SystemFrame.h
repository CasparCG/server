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