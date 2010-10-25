#include "..\..\StdAfx.h"

#include "image_loader.h"

#include "../../../common/exception/Exceptions.h"

#include "../../frame/gpu_frame.h"
#include "../../../common/image/image.h"

#if defined(_MSC_VER)
#pragma warning (disable : 4714) // marked as __forceinline not inlined
#endif

#include <boost/filesystem.hpp>
#include <boost/exception/errinfo_file_name.hpp>

namespace caspar{ namespace image{

std::shared_ptr<FIBITMAP> load_image(const std::string& filename)
{
	if(!boost::filesystem::exists(filename))
		BOOST_THROW_EXCEPTION(file_not_found() << boost::errinfo_file_name(filename));

	FREE_IMAGE_FORMAT fif = FIF_UNKNOWN;
	fif = FreeImage_GetFileType(filename.c_str(), 0);
	if(fif == FIF_UNKNOWN) 
		fif = FreeImage_GetFIFFromFilename(filename.c_str());
		
	if(fif == FIF_UNKNOWN || !FreeImage_FIFSupportsReading(fif)) 
		BOOST_THROW_EXCEPTION(invalid_argument() << msg_info("Unsupported image format."));
		
	auto bitmap = std::shared_ptr<FIBITMAP>(FreeImage_Load(fif, filename.c_str(), 0), FreeImage_Unload);
	//FREE_IMAGE_TYPE ImageType = FreeImage_GetImageType(bitmap.get());
		  
	if(FreeImage_GetBPP(bitmap.get()) != 32)
	{
		bitmap = std::shared_ptr<FIBITMAP>(FreeImage_ConvertTo32Bits(bitmap.get()), FreeImage_Unload);
		if(!bitmap)
			BOOST_THROW_EXCEPTION(invalid_argument() << msg_info("Unsupported image format."));			
	}
	
	return bitmap;
}

std::shared_ptr<FIBITMAP> load_image(const std::wstring& filename)
{
	return load_image(common::narrow(filename));
}

}}