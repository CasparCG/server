#pragma once

#include "basic_frame.h"

#include "../../video_format.h"

#include <boost/noncopyable.hpp>
#include <boost/range/iterator_range.hpp>

#include <memory>
#include <vector>

namespace caspar { namespace core {
	
class write_frame : public basic_frame, boost::noncopyable
{
public:			
	virtual boost::iterator_range<unsigned char*> image_data(size_t plane_index = 0) = 0;
	virtual std::vector<short>& audio_data() = 0;
	
	virtual const boost::iterator_range<const unsigned char*> image_data(size_t plane_index = 0) const = 0;
	virtual const boost::iterator_range<const short*> audio_data() const = 0;

	virtual void accept(frame_visitor& visitor) = 0;

	virtual int tag() const = 0;
};

}}