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
 
#pragma once

#include "..\..\MediaManager.h"
#include "..\..\MediaProducer.h"

#include <string>

namespace caspar {

class FileInfo;

class ColorManager : public IMediaManager
{
public:
	ColorManager() {}
	virtual ~ColorManager() {}

	virtual MediaProducerPtr CreateProducer(const tstring& parameter);
	virtual bool getFileInfo(FileInfo* pFileInfo);

	static bool GetPixelColorValueFromString(const tstring& parameter, unsigned long* outValue);
};

}	//namespace caspar
