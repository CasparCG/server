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

#ifndef _BEFILEINPUTSTREAM_H__
#define _BEFILEINPUTSTREAM_H__

#pragma once

#include <string>
#include <fstream>

namespace caspar {
namespace psd {

class UnexpectedEOFException : public std::exception
{
public:
	virtual ~UnexpectedEOFException()
	{}
	virtual const char *what() const
	{
		return "Unexpected end of file";
	}
};
class FileNotFoundException : public std::exception
{
public:
	virtual ~FileNotFoundException()
	{}
	virtual const char *what() const
	{
		return "File not found";
	}
};

class BEFileInputStream
{
public:
	explicit BEFileInputStream();
	virtual ~BEFileInputStream();

	void Open(const std::wstring& filename);

	void read(char*, std::streamsize);
	unsigned char read_byte();
	unsigned short read_short();
	unsigned long read_long();
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
	std::ifstream	ifs_;
	std::wstring	filename_;
};

class StreamPositionBackup
{
public:
	StreamPositionBackup(BEFileInputStream* pStream, std::streamoff newPos) : pStream_(pStream)
	{
		oldPosition_ = pStream->current_position();
		pStream_->set_position(newPos);
	}
	~StreamPositionBackup()
	{
		pStream_->set_position(oldPosition_);
	}
private:
	std::streamoff oldPosition_;
	BEFileInputStream* pStream_;

};

}	//namespace psd
}	//namespace caspar

#endif	//_BEFILEINPUTSTREAM_H__