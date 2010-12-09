#pragma once

#include "../format/video_format.h"
#include "../format/pixel_format.h"

#include <boost/noncopyable.hpp>
#include <boost/range/iterator_range.hpp>

#include <memory>
#include <vector>

namespace caspar { namespace core {

class read_frame : boost::noncopyable
{
public:	
	explicit read_frame(size_t width, size_t height);

	const boost::iterator_range<const unsigned char*> data() const;
	const std::vector<short>& audio_data() const;
	std::vector<short>& audio_data();
	
	void begin_read();
	void end_read();

private:
	struct implementation;
	std::shared_ptr<implementation> impl_;
};
typedef std::shared_ptr<read_frame> read_frame_ptr;
		
class consumer_frame
{
public:	
	consumer_frame(const read_frame_ptr& frame) : frame_(frame){}
	const boost::iterator_range<const unsigned char*> data() const {return frame_->data();}
	const std::vector<short>& audio_data() const {return frame_->audio_data();}
	
private:
	read_frame_ptr frame_;
};
typedef std::shared_ptr<consumer_frame> consumer_frame_ptr;
typedef std::unique_ptr<consumer_frame> consumer_frame_uptr;

}}