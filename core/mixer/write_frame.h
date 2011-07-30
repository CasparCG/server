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

#include <core/producer/frame/basic_frame.h>
#include <core/video_format.h>

#include <boost/noncopyable.hpp>
#include <boost/range/iterator_range.hpp>

#include <cstdint>
#include <vector>

namespace caspar { namespace core {

class host_buffer;
class device_buffer;
struct frame_visitor;
struct pixel_format_desc;
class ogl_device;	

class write_frame : public core::basic_frame, boost::noncopyable
{
public:	
	write_frame(const void* tag);
	explicit write_frame(ogl_device& ogl, const void* tag, const core::pixel_format_desc& desc);
	write_frame(const write_frame& other);
			
	virtual boost::iterator_range<uint8_t*> image_data(size_t plane_index = 0);	
	virtual std::vector<int16_t>& audio_data();
	
	virtual const boost::iterator_range<const uint8_t*> image_data(size_t plane_index = 0) const;
	virtual const boost::iterator_range<const int16_t*> audio_data() const;

	void commit(uint32_t plane_index);
	void commit();
	
	void set_type(const core::video_mode::type& mode);
	core::video_mode::type get_type() const;
	
	virtual void accept(core::frame_visitor& visitor);

	virtual const void* tag() const;

	const core::pixel_format_desc& get_pixel_format_desc() const;
	
private:
	friend class image_mixer;
	
	const std::vector<safe_ptr<device_buffer>>& get_textures() const;

	struct implementation;
	safe_ptr<implementation> impl_;
};


}}