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

	auto tmp2 = fileStream.tellg();
	tmp2;

    fileStream.read(bytes.data(), 8);
	
	auto tmp3 = fileStream.gcount();
	tmp3;

	tmp2 = fileStream.tellg();
	tmp2;

	fileStream.seekg(1, std::ios::cur);
	
	tmp2 = fileStream.tellg();
	tmp2;

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
		
		std::vector<char> bytes2(256);
		fileStream.read(bytes2.data(), bytes2.size());

		auto ptr = bytes2.data();
		
		ptr += 27;
						
		if(std::string(ptr, ptr+10) == "onMetaData")
		{
			ptr += 16;

			for(int n = 0; n < 16; ++n)
			{
				char name_size = *ptr++;

				if(name_size == 0)
					break;

				auto name = std::string(ptr, ptr + name_size);
				ptr += name_size;

				char data_type = *ptr++;
				switch(data_type)
				{
				case 0:
					{
						std::reverse(ptr, ptr+8);
						values[name] = boost::lexical_cast<std::string>(*(double*)(ptr));
						ptr += 9;

						break;
					}
				case 1:
					{
						values[name] = boost::lexical_cast<std::string>(*ptr != 0);
						ptr += 2;

						break;
					}
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