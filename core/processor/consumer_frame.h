#pragma once

#include "read_frame.h"

#include <boost/noncopyable.hpp>
#include <boost/range/iterator_range.hpp>

#include <memory>
#include <vector>

namespace caspar { namespace core {

class consumer_frame
{
public:	
	consumer_frame(read_frame&& frame) : frame_(std::move(frame)){}
	const boost::iterator_range<const unsigned char*> pixel_data() const {return frame_.pixel_data();}
	const std::vector<short>& audio_data() const {return frame_.audio_data();}
	
private:
	read_frame frame_;
};
typedef std::shared_ptr<consumer_frame> consumer_frame_ptr;
typedef std::unique_ptr<consumer_frame> consumer_frame_uptr;

}}