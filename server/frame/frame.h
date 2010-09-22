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

#include <memory>
#include <vector>

#include "audio_chunk.h"

namespace caspar {

// NOTE: audio data is ALWAYS shallow copy
class frame : boost::noncopyable
{
public:
	virtual ~frame(){}
	
	virtual const unsigned char* data() const { return const_cast<frame&>(*this).data(); }
	virtual unsigned char* data() = 0;
	virtual size_t size() const = 0;
	virtual size_t width() const = 0;
	virtual size_t height() const = 0;
	virtual void* tag() const { return nullptr; }
	virtual const std::vector<audio_chunk_ptr>& audio_data() const { return audioData_; }	
	virtual std::vector<audio_chunk_ptr>& audio_data() { return audioData_; }			

	static std::shared_ptr<frame> null()
	{
		class null_frame : public frame
		{
			unsigned char* data() { return nullptr; };
			size_t size() const { return 0; };
			size_t width() const { return 0; };
			size_t height() const { return 0; };
		};
		static auto my_null_frame = std::make_shared<null_frame>();
		return my_null_frame;
	}
private:	
	std::vector<audio_chunk_ptr> audioData_;
};
typedef std::shared_ptr<frame> frame_ptr;
typedef std::shared_ptr<const frame> frame_const_ptr;
typedef std::unique_ptr<frame> frame_uptr;
typedef std::unique_ptr<const frame> frame_const_uptr;

inline bool operator==(const frame& lhs, const frame& rhs)
{
	return lhs.data() == rhs.data() && (lhs.data() == nullptr || lhs.size() == rhs.size());
}

}