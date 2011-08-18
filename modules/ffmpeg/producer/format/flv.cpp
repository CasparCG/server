#include "../../stdafx.h"

#include "flv.h"

#include <common/exception/exceptions.h>

#include <boost/filesystem.hpp>

#include <iostream>

#include <unordered_map>

namespace caspar {
	
double next_double(std::fstream& fileStream)
{
	std::vector<char> bytes(8);
    fileStream.read(bytes.data(), bytes.size());
	fileStream.seekg(1, std::ios::cur);

	std::reverse(bytes.begin(), bytes.end());
	double* tmp = (double*)bytes.data();

    return *tmp;
} 

bool next_bool(std::fstream& fileStream)
{
	std::vector<char> bytes(1);
    fileStream.read(bytes.data(), bytes.size());
	fileStream.seekg(1, std::ios::cur);
    return bytes[0] != 0;
}

std::string next_string(std::fstream& fileStream)
{
	std::vector<char> bytes(256, 0);
	fileStream.seekg(2, std::ios::cur);
    fileStream.getline(bytes.data(), bytes.size(), 0);
    return std::string(bytes.begin(), bytes.end());
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
		
		if(std::string(bytes.begin(), bytes.end()) == "onMetaData")
		{
			fileStream.seekg(6, std::ios::cur);

			for(int n = 0; n < 16; ++n)
			{
				char name_size = 0;
				fileStream.read(&name_size, 1);

				if(name_size == 0)
					break;

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
				case 2:
					values[name_str] = next_string(fileStream);
					break;
				}
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