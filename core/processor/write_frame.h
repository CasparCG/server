#pragma once

#include "fwd.h"

#include "gpu_frame.h"

#include "../format/video_format.h"
#include "../format/pixel_format.h"

#include <boost/noncopyable.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/range/iterator_range.hpp>

#include <memory>
#include <array>
#include <vector>

namespace caspar { namespace core {
		
class write_frame : public gpu_frame
{
	friend class frame_renderer;
	friend class frame_processor_device;

public:	
	boost::iterator_range<unsigned char*> data(size_t index = 0);
	const boost::iterator_range<const unsigned char*> data(size_t index = 0) const;

	virtual std::vector<short>& audio_data();
	virtual const std::vector<short>& audio_data() const;
	
private:
	explicit write_frame(const pixel_format_desc& desc);

	virtual void reset();
		
	virtual void begin_write();
	virtual void end_write();
	virtual void draw(frame_shader& shader);

	struct implementation;
	std::shared_ptr<implementation> impl_;
};
typedef std::shared_ptr<write_frame> write_frame_ptr;


}}