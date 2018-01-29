#pragma once

#include <common/memory.h>

#include <core/fwd.h>

#include <boost/property_tree/ptree_fwd.hpp>

#include <string>
#include <vector>

namespace caspar { namespace ffmpeg {

spl::shared_ptr<core::frame_consumer> create_ffmpeg_consumer(
		const std::vector<std::wstring>& params, core::interaction_sink*, std::vector<spl::shared_ptr<core::video_channel>> channels);
spl::shared_ptr<core::frame_consumer> create_preconfigured_ffmpeg_consumer(
		const boost::property_tree::wptree& ptree, core::interaction_sink*, std::vector<spl::shared_ptr<core::video_channel>> channels);

}}
