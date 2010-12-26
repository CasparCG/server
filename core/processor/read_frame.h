#pragma once

#include "host_buffer.h"	

#include <boost/noncopyable.hpp>
#include <boost/range/iterator_range.hpp>

#include <memory>
#include <vector>

#include "../../common/utility/safe_ptr.h"

namespace caspar { namespace core {
	
class read_frame
{
public:
	read_frame();
	read_frame(safe_ptr<const host_buffer>&& image_data, std::vector<short>&& audio_data);

	const boost::iterator_range<const unsigned char*> image_data() const;
	const boost::iterator_range<const short*> audio_data() const;
	
private:
	struct implementation;
	std::shared_ptr<implementation> impl_;
};

}}