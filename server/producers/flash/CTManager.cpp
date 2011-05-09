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
 
#include "..\..\stdafx.h"

#include "..\..\frame\FrameManager.h"
#include "..\..\frame\FrameMediaController.h"
#include "..\..\utils\FileInputStream.h"
#include "..\..\fileinfo.h"
#include "CTManager.h"
#include "FlashProducer.h"
#include "..\..\cg\FlashCGProxy.h"

namespace caspar {

using namespace caspar::utils;
using namespace caspar::CG;

//////////////////////////////
//  CTManager definition
//
CTManager::CTManager()
{
	_extensions.push_back(TEXT("ct"));
}

CTManager::~CTManager()
{}

MediaProducerPtr CTManager::CreateProducer(const tstring& filename) {
	tstring fixedFilename = filename;
	tstring::size_type pos = 0;
	while((pos = fixedFilename.find(TEXT('\\'), pos)) != tstring::npos) {
		fixedFilename[pos] = TEXT('/');
	}
	MediaProducerPtr result;
	FlashCGProxyPtr pCGProxy(FlashCGProxy::Create());
	if(pCGProxy) {
		pCGProxy->Add(0, filename, 1);
		result = pCGProxy->GetFlashProducer();
	}

	return result;
}

bool CTManager::getFileInfo(FileInfo* pFileInfo)
{
	if(pFileInfo != 0) {
		pFileInfo->length = 1;
		pFileInfo->type = TEXT("movie");
		pFileInfo->encoding = TEXT("ct");
		return true;
	}
	return false;
}

}	//namespace caspar