#pragma once

#include <common/forward.h>

#include <core/frame/frame.h>

FORWARD2(caspar, core, class frame_factory);

namespace caspar { namespace accelerator { namespace cpu {
	
class deinterlacer sealed 
{
public:

	// Static Members

	// Constructors

	deinterlacer();
	~deinterlacer();

	// Methods	
			
	std::vector<core::const_frame> operator()(const core::const_frame& frame, core::frame_factory& frame_factory);
		
	// Properties

private:
	struct impl;
	spl::unique_ptr<impl> impl_;
};

}}}