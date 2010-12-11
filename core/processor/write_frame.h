#pragma once

#include "fwd.h"

#include "../format/video_format.h"
#include "../format/pixel_format.h"

#include <boost/noncopyable.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/range/iterator_range.hpp>

#include <memory>
#include <array>
#include <vector>

namespace caspar { namespace core {
		
class write_frame
{
public:	
	explicit write_frame(const pixel_format_desc& desc);

	boost::iterator_range<unsigned char*> pixel_data(size_t index = 0);
	const boost::iterator_range<const unsigned char*> pixel_data(size_t index = 0) const;

	std::vector<short>& audio_data();
	const std::vector<short>& audio_data() const;
	
	void reset();
		
	void begin_write();
	void end_write();
	void draw(frame_shader& shader);

private:
	struct implementation;
	std::shared_ptr<implementation> impl_;
};
typedef std::shared_ptr<write_frame> write_frame_ptr;


}}