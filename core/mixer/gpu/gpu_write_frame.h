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

#include <common/memory/safe_ptr.h>
#include <core/producer/frame/write_frame.h>
#include <core/producer/frame/pixel_format.h>

#include "../gpu/host_buffer.h"

#include <boost/noncopyable.hpp>
#include <boost/range/iterator_range.hpp>

#include <memory>
#include <vector>

namespace caspar { namespace mixer {
	
class gpu_write_frame : public core::write_frame
{
public:	
	explicit gpu_write_frame(int tag, const core::pixel_format_desc& desc, const std::vector<safe_ptr<host_buffer>>& buffers);
	gpu_write_frame(gpu_write_frame&& other);
	gpu_write_frame& operator=(gpu_write_frame&& other);
	
	void swap(gpu_write_frame& other);

	const core::pixel_format_desc& get_pixel_format_desc() const;
	std::vector<safe_ptr<host_buffer>>& get_plane_buffers();
		
	// core::write_frame
	virtual boost::iterator_range<unsigned char*> image_data(size_t plane_index = 0);	
	virtual std::vector<short>& audio_data();
	
	virtual const boost::iterator_range<const unsigned char*> image_data(size_t plane_index = 0) const;
	virtual const boost::iterator_range<const short*> audio_data() const;

	virtual void accept(core::frame_visitor& visitor);

	virtual int tag() const;
	
private:
	struct implementation;
	std::shared_ptr<implementation> impl_;
};
typedef std::shared_ptr<gpu_write_frame> gpu_write_frame_impl_ptr;


}}