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

#pragma once

#include <common/except.h>

#include <boost/filesystem/fstream.hpp>

#include <string>
#include <fstream>
#include <cstdint>

namespace caspar { namespace psd {

struct UnexpectedEOFException : virtual io_error {};

class bigendian_file_input_stream
{
public:
	explicit bigendian_file_input_stream();
	virtual ~bigendian_file_input_stream();

	void open(const std::wstring& filename);

	void read(char*, std::streamsize);
	std::uint8_t read_byte();
	std::uint16_t read_short();
	std::uint32_t read_long();
	std::wstring read_pascal_string(unsigned char padding = 1);
	std::wstring read_unicode_string();
	std::wstring read_id_string();
	double read_double();

	void discard_bytes(std::streamoff);
	void discard_to_next_word();
	void discard_to_next_dword();

	std::streamoff current_position();
	void set_position(std::streamoff);

	void close();
private:
	boost::filesystem::ifstream	ifs_;
	std::wstring				filename_;
};

class StreamPositionBackup
{
public:
	StreamPositionBackup(bigendian_file_input_stream& stream, std::streamoff newPos) : stream_(stream)
	{
		oldPosition_ = stream.current_position();
		stream_.set_position(newPos);
	}

	~StreamPositionBackup()
	{
		stream_.set_position(oldPosition_);
	}
private:
	std::streamoff					oldPosition_;
	bigendian_file_input_stream&	stream_;

};

}	//namespace psd
}	//namespace caspar
