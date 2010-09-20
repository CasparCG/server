#include "../../StdAfx.h"

#include "command.h"
#include "exception.h"

#include "../../frame/format.h"

#include "../../server.h"

#include <boost/regex.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/range.hpp>
#include <boost/date_time.hpp>
#include <boost/filesystem.hpp>

namespace caspar { namespace controller { namespace amcp {
		
std::wstring list_media()
{	
	std::wstringstream replyString;
	for (boost::filesystem::wrecursive_directory_iterator itr(server::media_folder()), end; itr != end; ++itr)
    {			
		if(boost::filesystem::is_regular_file(itr->path()))
		{
			std::wstring clipttype = TEXT(" N/A ");
			std::wstring extension = boost::to_upper_copy(itr->path().extension());
			if(extension == TEXT(".TGA") || extension == TEXT(".COL"))
				clipttype = TEXT(" STILL ");
			else if(extension == TEXT(".SWF") || extension == TEXT(".DV") || extension == TEXT(".MOV") || extension == TEXT(".MPG") || 
					extension == TEXT(".AVI") || extension == TEXT(".FLV") || extension == TEXT(".F4V") || extension == TEXT(".MP4"))
				clipttype = TEXT(" MOVIE ");
			else if(extension == TEXT(".WAV") || extension == TEXT(".MP3"))
				clipttype = TEXT(" STILL ");

			if(clipttype != TEXT(" N/A "))
			{		
				auto is_not_digit = [](char c){ return std::isdigit(c) == 0; };

				auto relativePath = boost::filesystem::wpath(itr->path().file_string().substr(server::media_folder().size()-1, itr->path().file_string().size()));

				auto writeTimeStr = boost::posix_time::to_iso_string(boost::posix_time::from_time_t(boost::filesystem::last_write_time(itr->path())));
				writeTimeStr.erase(std::remove_if(writeTimeStr.begin(), writeTimeStr.end(), is_not_digit), writeTimeStr.end());
				auto writeTimeWStr = std::wstring(writeTimeStr.begin(), writeTimeStr.end());

				auto sizeStr = boost::lexical_cast<std::wstring>(boost::filesystem::file_size(itr->path()));
				sizeStr.erase(std::remove_if(sizeStr.begin(), sizeStr.end(), is_not_digit), sizeStr.end());
				auto sizeWStr = std::wstring(sizeStr.begin(), sizeStr.end());

				replyString << TEXT("\"") << relativePath.replace_extension(TEXT(""))
							<< TEXT("\" ") << clipttype 
							<< TEXT(" ") << sizeStr
							<< TEXT(" ") << writeTimeWStr
							<< TEXT("\r\n");
			}	
		}
	}
	return boost::to_upper_copy(replyString.str());
}

std::wstring list_templates() 
{
	std::wstringstream replyString;

	for (boost::filesystem::wrecursive_directory_iterator itr(server::template_folder()), end; itr != end; ++itr)
    {		
		if(boost::filesystem::is_regular_file(itr->path()) && itr->path().extension() == L".ft")
		{
			auto relativePath = boost::filesystem::wpath(itr->path().file_string().substr(server::template_folder().size()-1, itr->path().file_string().size()));

			auto writeTimeStr = boost::posix_time::to_iso_string(boost::posix_time::from_time_t(boost::filesystem::last_write_time(itr->path())));
			writeTimeStr.erase(std::remove_if(writeTimeStr.begin(), writeTimeStr.end(), [](char c){ return std::isdigit(c) == 0;}), writeTimeStr.end());
			auto writeTimeWStr = std::wstring(writeTimeStr.begin(), writeTimeStr.end());

			auto sizeStr = boost::lexical_cast<std::string>(boost::filesystem::file_size(itr->path()));
			sizeStr.erase(std::remove_if(sizeStr.begin(), sizeStr.end(), [](char c){ return std::isdigit(c) == 0;}), sizeStr.end());

			auto sizeWStr = std::wstring(sizeStr.begin(), sizeStr.end());
			
			replyString << TEXT("\"") << relativePath.replace_extension(TEXT(""))
						<< TEXT("\" ") << sizeWStr
						<< TEXT(" ") << writeTimeWStr
						<< TEXT("\r\n");		
		}
	}
	return boost::to_upper_copy(replyString.str());
}

}}}
