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
 
#include "..\..\StdAfx.h"

#include "TargaScrollManager.h"
#include "TargaScrollProducer.h"
#include "..\..\FileInfo.h"
#include "..\..\MediaManager.h"

namespace caspar {

TargaScrollMediaManager::TargaScrollMediaManager() : IMediaManager()
{
	IMediaManager::_extensions.push_back(TEXT("stga"));
}

TargaScrollMediaManager::~TargaScrollMediaManager()
{
}

MediaProducerPtr TargaScrollMediaManager::CreateProducer(const tstring& filename)
{
	TargaScrollMediaProducerPtr pTargaScrollMediaProducer(new TargaScrollMediaProducer());
	if (!pTargaScrollMediaProducer->Load(filename))
		pTargaScrollMediaProducer.reset();

	return pTargaScrollMediaProducer;
}

bool TargaScrollMediaManager::getFileInfo(FileInfo* pFileInfo)
{
	pFileInfo->length = 1;
	pFileInfo->type = TEXT("movie");
	pFileInfo->encoding = TEXT("NA");

	return true;
}

}