#pragma once

#include <boost/noncopyable.hpp>
#include <boost/range/iterator_range.hpp>

#include <memory>
#include <vector>

#include <common/memory/safe_ptr.h>

namespace caspar { namespace core {
	
class read_frame
{
public:
	virtual const boost::iterator_range<const unsigned char*> image_data() const {return boost::iterator_range<const unsigned char*>();}
	virtual const boost::iterator_range<const short*> audio_data() const {return boost::iterator_range<const short*>();}

	static safe_ptr<const read_frame> empty()
	{
		return safe_ptr<const read_frame>();
	}
};

}}