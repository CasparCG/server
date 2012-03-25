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
* Author: Robert Nagy, ronag89@gmail.com
*/

#pragma once

#include <common/memory.h>

#include <memory>
#include <string>
#include <cstdint>

#include <boost/noncopyable.hpp>

struct AVFormatContext;
struct AVPacket;

namespace caspar {

namespace diagnostics {

class graph;

}
	 
namespace ffmpeg {

class input : boost::noncopyable
{
public:
	explicit input(const spl::shared_ptr<diagnostics::graph>& graph, const std::wstring& filename, bool loop, uint32_t start, uint32_t length);

	bool		try_pop_video(std::shared_ptr<AVPacket>& packet);
	bool		try_pop_audio(std::shared_ptr<AVPacket>& packet);

	void		loop(bool value);
	bool		loop() const;

	void		start(uint32_t value);
	uint32_t	start() const;

	void		length(uint32_t value);
	uint32_t	length() const;

	void		seek(uint32_t target);

	AVFormatContext& context();
private:
	struct impl;
	std::shared_ptr<impl> impl_;
};

	
}}
