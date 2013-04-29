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

#include <common/memory/safe_ptr.h>

#include <boost/noncopyable.hpp>

struct AVFormatContext;
struct AVFrame;
struct AVPacket;

namespace caspar {

namespace core {
	struct frame_factory;
	class write_frame;
}

namespace ffmpeg {

/*
PacketFrame stores both the AVPacket and the AVFrame produced by the decoder.

This is passed through to the frame muxer so that the AVPacket is not freed until after
a call to write_frame.  Some codecs, such as the RAW_VIDEO codec, simply return the
data from the AVPacket. Consequently, this data should not be freed until after it has
been used by write_frame.
*/
struct PacketFrame
{
public:
	PacketFrame(std::shared_ptr<AVPacket> const& p, std::shared_ptr<AVFrame> f) :
		packet(p),
		frame(f)
	{}

	static std::shared_ptr<PacketFrame> create(std::shared_ptr<AVPacket> const& p, std::shared_ptr<AVFrame> const& f) {
		return std::shared_ptr<PacketFrame>(new PacketFrame(p, f));
	}

	std::shared_ptr<AVPacket> packet;
	std::shared_ptr<AVFrame> frame;
};

class video_decoder : boost::noncopyable
{
public:
	explicit video_decoder(const safe_ptr<AVFormatContext>& context);
	
	bool ready() const;
	void push(const std::shared_ptr<AVPacket>& packet);
	std::shared_ptr<PacketFrame> poll();
	
	size_t	 width()		const;
	size_t	 height()	const;

	uint32_t nb_frames() const;
	uint32_t file_frame_number() const;

	bool	 is_progressive() const;

	std::wstring print() const;

private:
	struct implementation;
	safe_ptr<implementation> impl_;
};

}}