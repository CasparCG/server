/*
* copyright (c) 2010 Sveriges Television AB <info@casparcg.com>
*
*  This file is part of CasparCG.
*
*    CasparCG is free software: you can redistribute it and/or modify
*    it under the terms of the GNU General Public License as published by
*    the Free Software Foundation, either version 3 of the License, or
*    (at your option) any later version.
*
*    CasparCG is distributed in the hope that it will be useful,
*    but WITHOUT ANY WARRANTY; without even the implied warranty of
*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*    GNU General Public License for more details.

*    You should have received a copy of the GNU General Public License
*    along with CasparCG.  If not, see <http://www.gnu.org/licenses/>.
*
*/
#pragma once

#include <common/diagnostics/graph.h>

#include <memory>
#include <string>

#include <boost/iterator/iterator_facade.hpp>

struct AVCodecContext;

namespace caspar {
	
class input : boost::noncopyable
{
public:
	explicit input(const safe_ptr<diagnostics::graph>& graph, const std::wstring& filename, bool loop, int start, int length);
	const std::shared_ptr<AVCodecContext>& get_video_codec_context() const;
	const std::shared_ptr<AVCodecContext>& get_audio_codec_context() const;

	bool try_pop_video_packet(std::shared_ptr<AVPacket>& packet);
	bool try_pop_audio_packet(std::shared_ptr<AVPacket>& packet);

	bool is_running() const;
	double fps() const;
private:
	struct implementation;
	std::shared_ptr<implementation> impl_;
};
//
//class input_video_iterator : public boost::iterator_facade<input_video_iterator, std::shared_ptr<AVPacket>, boost::forward_traversal_tag>
//{
//	std::shared_ptr<AVPacket> pkt_;
//	input* input_;
//public:
//	input_video_iterator() : input_(nullptr){}
//
//    input_video_iterator(input& input)
//      : input_(&input) {}
//
//    input_video_iterator(const input_video_iterator& other)
//      : input_(other.input_) {}
//
// private:
//    friend class boost::iterator_core_access;
//
//    void increment() 
//	{
//		if(input_ && !input_->try_pop_video_packet(pkt_))
//			input_ = nullptr;
//	}
//
//    bool equal(input_video_iterator const& other) const
//    {
//        return input_ == other.input_;
//    }
//
//    std::shared_ptr<AVPacket> const& dereference() const { return pkt_; }
//};

	
}
