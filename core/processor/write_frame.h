#pragma once

#include "fwd.h"

#include "draw_frame.h"

#include "../../common/exception/exceptions.h"

#include "../format/video_format.h"
#include "../format/pixel_format.h"

#include <boost/noncopyable.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/range/iterator_range.hpp>

#include <memory>
#include <array>
#include <vector>

namespace caspar { namespace core {
		
class write_frame_impl
{
public:	
	explicit write_frame_impl(const pixel_format_desc& desc);

	boost::iterator_range<unsigned char*> pixel_data(size_t index = 0);
	const boost::iterator_range<const unsigned char*> pixel_data(size_t index = 0) const;
	
	void reset();

	std::vector<short>& audio_data();
	const std::vector<short>& audio_data() const;
		
	void begin_write();
	void end_write();
	void draw(frame_shader& shader);
	
private:
	struct implementation;
	std::shared_ptr<implementation> impl_;
};
typedef std::shared_ptr<write_frame_impl> write_frame_impl_ptr;

class write_frame : public draw_frame_impl
{
public:
	explicit write_frame(const write_frame_impl_ptr& frame) : impl_(frame)
	{
		if(!frame)
			BOOST_THROW_EXCEPTION(null_argument() << msg_info("frame"));
	}
	write_frame(write_frame&& other) : impl_(std::move(other.impl_)){}
	write_frame& operator=(write_frame&& other)
	{
		impl_ = std::move(other.impl_);
		return *this;
	}

	boost::iterator_range<unsigned char*> pixel_data(size_t index = 0) {return impl_->pixel_data(index);}
	const boost::iterator_range<const unsigned char*> pixel_data(size_t index = 0) const {return impl_->pixel_data(index);}
	
	virtual const std::vector<short>& audio_data() const {return impl_->audio_data();}
			
private:
	virtual void begin_write() {return impl_->begin_write();}
	virtual void end_write() {return impl_->end_write();}
	virtual void draw(frame_shader& shader) {return impl_->draw(shader);}

	write_frame_impl_ptr impl_;
};


}}