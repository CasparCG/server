#include "../StdAfx.h"

#include <common/exception/exceptions.h>

#include <fstream>
#include <streambuf>

#include <zlib.h>

namespace caspar { namespace flash {
	
std::vector<char> decompress_one_file(const std::vector<char>& in_data, uLong buf_size = 5000000)
{
	if(buf_size > 300*1000000)
		BOOST_THROW_EXCEPTION(file_read_error());

	std::vector<char> out_data(buf_size, 0);

	auto ret = uncompress((Bytef*)out_data.data(), &buf_size, (Bytef*)in_data.data(), in_data.size());

	if(ret == Z_BUF_ERROR)
		return decompress_one_file(in_data, buf_size*2);

	if(ret != Z_OK)
		BOOST_THROW_EXCEPTION(file_read_error());

	out_data.resize(buf_size);

	return out_data;
}

std::wstring read_template_meta_info(const std::wstring& filename)
{
	auto file = std::fstream(filename, std::ios::in | std::ios::binary);

	if(!file)
		BOOST_THROW_EXCEPTION(file_read_error());
	
	char head[4] = {};
	file.read(head, 3);
	
	std::vector<char> data;
	
	file.seekg(0, std::ios::end);   
	data.reserve(static_cast<size_t>(file.tellg()));
	file.seekg(0, std::ios::beg);

	if(strcmp(head, "CWS") == 0)
	{
		file.seekg(8, std::ios::beg);
		std::copy((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>(), std::back_inserter(data));
		data = decompress_one_file(data);
	}
	else
	{
		file.seekg(0, std::ios::end);   
		data.reserve(static_cast<size_t>(file.tellg()));
		file.seekg(0, std::ios::beg);

		std::copy((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>(), std::back_inserter(data));
	}
	
	std::string beg_str = "<template version";
	std::string end_str = "</template>";
	auto beg_it = std::find_end(data.begin(), data.end(), beg_str.begin(), beg_str.end());
	auto end_it = std::find_end(beg_it, data.end(), end_str.begin(), end_str.end());
	
	if(beg_it == data.end() || end_it == data.end())
		BOOST_THROW_EXCEPTION(file_read_error());
	
	return widen(std::string(beg_it, end_it+end_str.size()));
}

}}