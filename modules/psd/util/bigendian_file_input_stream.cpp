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
#include "..\..\..\common\utf.h"
#include <common/endian.h>

namespace caspar { namespace psd {

BEFileInputStream::BEFileInputStream()
{
}

BEFileInputStream::~BEFileInputStream()
{
	close();
}

void BEFileInputStream::Open(const std::wstring& filename)
{
	filename_ = filename;
	ifs_.open(filename_.c_str(), std::ios::in | std::ios::binary);
	if(!ifs_.is_open())
		throw FileNotFoundException();
}

void BEFileInputStream::close()
{
	if(ifs_.is_open())
		ifs_.close();
}

unsigned char BEFileInputStream::read_byte()
{
	unsigned char out;
	read((char*)&out, 1);

	return out;
}

unsigned short BEFileInputStream::read_short()
{
	unsigned short out;
	read((char*)&out, 2);

	return caspar::swap_byte_order(out);
}

unsigned long BEFileInputStream::read_long()
{
	unsigned long in;
	read((char*)&in, 4);

	return caspar::swap_byte_order(in);
}
double BEFileInputStream::read_double()
{
	char data[8];
	for(int i = 0; i < 8; ++i)
		data[7-i] = read_byte();

	return *reinterpret_cast<double*>(data);
}

void BEFileInputStream::read(char* buf, std::streamsize length)
{
	if(length > 0)
	{
		if(ifs_.eof())
			throw UnexpectedEOFException();

			ifs_.read(buf, length);
		
		if(ifs_.gcount() < length)
			throw UnexpectedEOFException();
	}
}

std::streamoff BEFileInputStream::current_position()
{
	return ifs_.tellg();
}

void BEFileInputStream::set_position(std::streamoff offset)
{
	ifs_.seekg(offset, std::ios_base::beg);
}

void BEFileInputStream::discard_bytes(std::streamoff length)
{
	ifs_.seekg(length, std::ios_base::cur);
}
void BEFileInputStream::discard_to_next_word()
{
	unsigned const char padding = 2;
	discard_bytes((padding - (current_position() % padding)) % padding);
}

void BEFileInputStream::discard_to_next_dword()
{
	unsigned const char padding = 4;
	discard_bytes((padding - (current_position() % padding)) % padding);
}

std::wstring BEFileInputStream::read_pascal_string(unsigned char padding)
{
	char strBuffer[256];

	unsigned char strLength = this->read_byte();

	strBuffer[strLength] = 0;
	this->read(strBuffer, strLength);

	unsigned char padded_bytes = (padding - ((strLength+1) % padding)) % padding;
	this->discard_bytes(padded_bytes);

	return caspar::u16(strBuffer);


}

std::wstring BEFileInputStream::read_unicode_string()
{
	unsigned long length = read_long();
	std::wstring result;

	if(length > 0)
	{
		result.reserve(length);

		//can be optimized. Reads and swaps byte-order, one char at the time
		for (unsigned long i = 0; i < length; ++i)
			result.append(1, static_cast<wchar_t>(read_short()));
	}

	return result;
}

std::wstring BEFileInputStream::read_id_string()
{
	std::string result;
	unsigned long length = read_long();
	
	if(length > 0)
	{
		result.reserve(length);

		for (unsigned long i = 0; i < length; ++i)
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
