#pragma once

#include <common/memory/safe_ptr.h>

#include <core/fwd.h>

#include <boost/property_tree/ptree.hpp>

#include <string>
#include <vector>

namespace caspar { namespace ffmpeg {
	
safe_ptr<core::frame_consumer> create_streaming_consumer(
		const core::parameters& params);
safe_ptr<core::frame_consumer> create_streaming_consumer(
		const boost::property_tree::wptree& ptree);

}}