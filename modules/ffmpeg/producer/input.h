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

#include "util.h"

#include <common/memory/safe_ptr.h>

#include <agents.h>
#include <concrt.h>

#include <memory>
#include <string>

#include <boost/noncopyable.hpp>
#include <boost/range/iterator_range.hpp>

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
	
	typedef Concurrency::ITarget<packet_message_t> target_t;

	explicit input(target_t& target, 
				   const safe_ptr<diagnostics::graph>& graph, 
				   const std::wstring& filename, bool loop, 
				   size_t start = 0, 
				   size_t length = std::numeric_limits<size_t>::max());
		
	size_t nb_frames() const;
	size_t nb_loops() const;
	
	safe_ptr<AVFormatContext> context();

	void start();
	void stop();
private:
	friend struct implemenation;
	struct implementation;
	std::shared_ptr<implementation> impl_;
};

	
}}
