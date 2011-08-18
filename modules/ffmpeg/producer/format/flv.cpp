#include "../../stdafx.h"

#include "flv.h"

#include <common/exception/exceptions.h>

#include <boost/filesystem.hpp>

#include <iostream>

namespace caspar {
	
double to_double(std::vector<char> bytes, bool readInReverse)
{
    if(bytes.size() != 8)
		BOOST_THROW_EXCEPTION(caspar_exception());

    if (readInReverse)
		std::reverse(bytes.begin(), bytes.end());
	
	static_assert(sizeof(double) == 8, "");

	double* tmp = (double*)bytes.data();
	
	double val = *tmp;
    return val;
}

double next_double(std::fstream& fileStream, int offset, int length)
{
    fileStream.seekg(offset, std::ios::cur);
	std::vector<char> bytes(length);
    fileStream.read(bytes.data(), length);
    return to_double(bytes, true);
}

flv_meta_info read_flv_meta_info(const std::wstring& filename)
{
	if(!boost::filesystem::exists(filename))
		BOOST_THROW_EXCEPTION(caspar_exception());

	flv_meta_info meta_info;

    std::fstream fileStream = std::fstream(narrow(filename), std::fstream::in);
	fileStream.seekg(27, std::ios::beg);
	
    std::array<char, 10> bytes;
    fileStream.read(bytes.data(), bytes.size());

	auto on_meta_data = std::string(bytes.begin(), bytes.end());   
	if (on_meta_data == "onMetaData")
    {
        //// 16 bytes past "onMetaData" is the data for "duration" 
        meta_info.duration = next_double(fileStream, 16, 8);
        //// 8 bytes past "duration" is the data for "width"
        meta_info.width = next_double(fileStream, 8, 8);
        //// 9 bytes past "width" is the data for "height"
        meta_info.height = next_double(fileStream, 9, 8);
        //// 16 bytes past "height" is the data for "videoDataRate"
        meta_info.video_data_rate = next_double(fileStream, 16, 8);
        //// 16 bytes past "videoDataRate" is the data for "audioDataRate"
        meta_info.audio_data_rate = next_double(fileStream, 16, 8);
        //// 12 bytes past "audioDataRate" is the data for "frameRate"
        meta_info.frame_rate = next_double(fileStream, 12, 8);
    }
    
    return meta_info;
}

}