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
 
#include "../../stdafx.h"

#include "FFMPEGManager.h"
#include "FFMPEGProducer.h"

#include "../../MediaProducerInfo.h"
#include "../../frame/FrameManager.h"
#include "../../frame/FrameMediaController.h"
#include "../../utils/UnhandledException.h"

#include <boost/thread/once.hpp>
#include <boost/assign.hpp>

namespace caspar {
namespace ffmpeg {

using namespace utils;
using namespace boost::assign;

FFMPEGManager::FFMPEGManager() 
{
	static boost::once_flag flag = BOOST_ONCE_INIT;
	boost::call_once(av_register_all, flag);
	
	_extensions += TEXT("mpg"), TEXT("avi"), TEXT("mov"), TEXT("dv"), TEXT("wav"), TEXT("mp3"), TEXT("mp4"), TEXT("f4v"), TEXT("flv");
}

MediaProducerPtr FFMPEGManager::CreateProducer(const tstring& filename)
{
	CASPAR_TRY
	{
		return std::make_shared<FFMPEGProducer>(filename);
	}
	CASPAR_CATCH_AND_LOG("[FFMPEGManager::CreateProducer]")

	return nullptr;
}

bool FFMPEGManager::getFileInfo(FileInfo* pFileInfo)
{
	if(pFileInfo == nullptr)
		return false;

	auto movie = list_of(TEXT("avi"))(TEXT("mpg"))(TEXT("mov"))(TEXT("dv"))(TEXT("flv"))(TEXT("f4v"))(TEXT("mp4"));
	auto audio = list_of(TEXT("wav"))(TEXT("mp3"));

	if(std::find(movie.begin(), movie.end(), pFileInfo->filetype) != movie.end())
	{
		pFileInfo->length = 0;	//get real length from file
		pFileInfo->type = TEXT("movie");
		pFileInfo->encoding = TEXT("codec");
	}
	else if(std::find(audio.begin(), audio.end(), pFileInfo->filetype) != audio.end()) 
	{
		pFileInfo->length = 0;	//get real length from file
		pFileInfo->type = TEXT("audio");
		pFileInfo->encoding = TEXT("NA");
	}
	return true;
}

}}