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
* Author: Niklas P Andersson, niklas.p.andersson@svt.se
*/

#include "bigendian_file_input_stream.h"

#include <common/utf.h>
#include <common/endian.h>

namespace caspar { namespace psd {

bigendian_file_input_stream::bigendian_file_input_stream()
{
}

bigendian_file_input_stream::~bigendian_file_input_stream()
{
	close();
}

void bigendian_file_input_stream::open(const std::wstring& filename)
{
	filename_ = filename;
	ifs_.open(filename_.c_str(), std::ios::in | std::ios::binary);
	if(!ifs_.is_open())
		CASPAR_THROW_EXCEPTION(file_not_found());
}

void bigendian_file_input_stream::close()
{
	if(ifs_.is_open())
		ifs_.close();
}

std::uint8_t bigendian_file_input_stream::read_byte()
{
	std::uint8_t out;
	read(reinterpret_cast<char*>(&out), 1);

	return out;
}

std::uint16_t bigendian_file_input_stream::read_short()
{
	std::uint16_t out;
	read(reinterpret_cast<char*>(&out), 2);

	return caspar::swap_byte_order(out);
}

std::uint32_t bigendian_file_input_stream::read_long()
{
	std::uint32_t in;
	read(reinterpret_cast<char*>(&in), 4);

	return caspar::swap_byte_order(in);
}

double bigendian_file_input_stream::read_double()
{
	char data[8];
	for(int i = 0; i < 8; ++i)
		data[7-i] = read_byte();

	return *reinterpret_cast<double*>(data);
}

void bigendian_file_input_stream::read(char* buf, std::streamsize length)
{
	if (length > 0)
	{
		if (ifs_.eof())
			CASPAR_THROW_EXCEPTION(unexpected_eof_exception());

		ifs_.read(buf, length);
		
		if (ifs_.gcount() < length)
			CASPAR_THROW_EXCEPTION(unexpected_eof_exception());
	}
}

std::streamoff bigendian_file_input_stream::current_position()
{
	return ifs_.tellg();
}

void bigendian_file_input_stream::set_position(std::streamoff offset)
{
	ifs_.seekg(offset, std::ios_base::beg);
}

void bigendian_file_input_stream::discard_bytes(std::streamoff length)
{
	ifs_.seekg(length, std::ios_base::cur);
}
void bigendian_file_input_stream::discard_to_next_word()
{
	const std::uint8_t padding = 2;
	discard_bytes((padding - (current_position() % padding)) % padding);
}

void bigendian_file_input_stream::discard_to_next_dword()
{
	const std::uint8_t padding = 4;
	discard_bytes((padding - (current_position() % padding)) % padding);
}

std::wstring bigendian_file_input_stream::read_pascal_string(std::uint8_t padding)
{
	char buffer[256];

	auto length = this->read_byte();

	buffer[length] = 0;
	this->read(buffer, length);

	auto padded_bytes = (padding - ((length + 1) % padding)) % padding;
	this->discard_bytes(padded_bytes);

	return caspar::u16(buffer);
}

std::wstring bigendian_file_input_stream::read_unicode_string()
{
	auto length = read_long();
	std::wstring result;

	if(length > 0)
	{
		result.reserve(length);

		//can be optimized. Reads and swaps byte-order, one char at the time
		for (std::uint32_t i = 0; i < length; ++i)
			result.append(1, static_cast<wchar_t>(read_short()));
	}

	return result;
}

std::wstring bigendian_file_input_stream::read_id_string()
{
	std::string result;
	auto length = read_long();
	
	if(length > 0)
	{
		result.reserve(length);

		for (std::uint32_t i = 0; i < length; ++i)
			result.append(1, read_byte());
	}
	else
	{
		result.reserve(4);
		for(int i = 0; i < 4; ++i)
			result.append(1, read_byte());
	}

	return caspar::u16(result);
}

}	//namespace psd
}	//namespace caspar
