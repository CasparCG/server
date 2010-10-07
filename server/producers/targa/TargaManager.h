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