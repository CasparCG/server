/*
* Copyright (c) 2011 Sveriges Television AB <info@casparcg.com>
*
* This file is part of CasparCG (www.casparcg.com).
*
* CasparCG is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* CasparCG is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with CasparCG. If not, see <http://www.gnu.org/licenses/>.
*
* Author: Robert Nagy, ronag89@gmail.com
*/

#include "image_loader.h"

#include <common/exception/Exceptions.h>
#include <common/utility/string.h>

#if defined(_MSC_VER)
#pragma warning (disable : 4714) // marked as __forceinline not inlined
#endif

#include <boost/exception/errinfo_file_name.hpp>
#include <boost/filesystem.hpp>

namespace caspar { namespace image {

std::shared_ptr<FIBITMAP> load_image(const std::wstring& filename)
{
	if(!boost::filesystem::exists(filename))
		BOOST_THROW_EXCEPTION(file_not_found() << boost::errinfo_file_name(narrow(filename)));

	FREE_IMAGE_FORMAT fif = FIF_UNKNOWN;
	fif = FreeImage_GetFileTypeU(filename.c_str(), 0);
	if(fif == FIF_UNKNOWN) 
		fif = FreeImage_GetFIFFromFilenameU(filename.c_str());
		
	if(fif == FIF_UNKNOWN || !FreeImage_FIFSupportsReading(fif)) 
		BOOST_THROW_EXCEPTION(invalid_argument() << msg_info("Unsupported image format."));
		
	auto bitmap = std::shared_ptr<FIBITMAP>(FreeImage_LoadU(fif, filename.c_str(), 0), FreeImage_Unload);
		  
	if(FreeImage_GetBPP(bitmap.get()) != 32)
	{
		bitmap = std::shared_ptr<FIBITMAP>(FreeImage_ConvertTo32Bits(bitmap.get()), FreeImage_Unload);
		if(!bitmap)
			BOOST_THROW_EXCEPTION(invalid_argument() << msg_info("Unsupported image format."));			
	}
	
	return bitmap;
}

std::shared_ptr<FIBITMAP> load_image(const std::string& filename)
{
	return load_image(widen(filename));
}

}}