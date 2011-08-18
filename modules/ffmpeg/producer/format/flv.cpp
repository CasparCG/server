#include "../../stdafx.h"

#include "flv.h"

#include <common/exception/exceptions.h>

#include <boost/filesystem.hpp>

#include <iostream>

#include <unordered_map>

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

double next_double(std::fstream& fileStream)
{
	std::vector<char> bytes(8);
    fileStream.read(bytes.data(), bytes.size());
    return to_double(bytes, true);
} 

bool next_bool(std::fstream& fileStream)
{
	std::vector<char> bytes(1);
    fileStream.read(bytes.data(), bytes.size());
    return bytes[0] != 0;
}

std::map<std::string, std::string> read_flv_meta_info(const std::string& filename)
{
	std::map<std::string, std::string>  values;

	if(boost::filesystem2::path(filename).extension() != ".flv")
		return values;
	
	try
	{
		if(!boost::filesystem2::exists(filename))
			BOOST_THROW_EXCEPTION(caspar_exception());
	
		std::fstream fileStream = std::fstream(filename, std::fstream::in);
		fileStream.seekg(27, std::ios::beg);
	
		std::vector<char> bytes(10);
		fileStream.read(bytes.data(), bytes.size());
		
		if (std::string(bytes.begin(), bytes.end()) == "onMetaData")
		{
			fileStream.seekg(6, std::ios::cur);

			for(int n = 0; n < 9; ++n)
			{
				char name_size = 0;
				fileStream.read(&name_size, 1);

				std::vector<char> name(name_size);
				fileStream.read(name.data(), name.size());
				auto name_str = std::string(name.begin(), name.end());

				char data_type = 0;
				fileStream.read(&data_type, 1);

				switch(data_type)
				{
				case 0:
					values[name_str] = boost::lexical_cast<std::string>(next_double(fileStream));
					break;
				case 1:
					values[name_str] = boost::lexical_cast<std::string>(next_bool(fileStream));
					break;
				}
				fileStream.seekg(1, std::ios::cur);
			}
		}
	}
	catch(...)
	{
		CASPAR_LOG_CURRENT_EXCEPTION();
	}

    return values;
}

}