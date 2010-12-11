#pragma once

#include "fwd.h"

#include "drawable_frame.h"

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

class write_frame : public drawable_frame
{
public:
	explicit write_frame(const write_frame_impl_ptr& frame) : frame_(frame){}
	write_frame(write_frame&& other) : frame_(std::move(other.frame_)){}
	write_frame& operator=(write_frame&& other)
	{
		frame_ = std::move(other.frame_);
		return *this;
	}

	boost::iterator_range<unsigned char*> pixel_data(size_t index = 0) {return frame_->pixel_data(index);}
	const boost::iterator_range<const unsigned char*> pixel_data(size_t index = 0) const {return frame_->pixel_data(index);}
	
	void reset() {return frame_->reset();}

	virtual std::vector<short>& audio_data() {return frame_->audio_data();}
	virtual const std::vector<short>& audio_data() const {return frame_->audio_data();}
			
	virtual void begin_write() {return frame_->begin_write();}
	virtual void end_write() {return frame_->end_write();}
	virtual void draw(frame_shader& shader) {return frame_->draw(shader);}
private:
	write_frame_impl_ptr frame_;
};


}}