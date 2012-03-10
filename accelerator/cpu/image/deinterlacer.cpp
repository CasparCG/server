#include "../../StdAfx.h"

#include "deinterlacer.h"

#include <core/frame/frame_factory.h>

#include <modules/ffmpeg/producer/filter/filter.h>
#include <modules/ffmpeg/producer/util/util.h>

#include <tbb/concurrent_hash_map.h>

#include <tuple>

namespace caspar { namespace accelerator { namespace cpu {

struct deinterlacer::impl
{
	ffmpeg::filter filter_;	

public:

	impl() 
		: filter_(L"YADIF=1:-1")
	{
	}

	std::vector<core::const_frame> operator()(const core::const_frame& frame, core::frame_factory& frame_factory)
	{		
		std::array<uint8_t*, 4> data = {};
		for(int n = 0; n < frame.pixel_format_desc().planes.size(); ++n)
			data[n] = const_cast<uint8_t*>(frame.image_data(n).begin());

		auto av_frame = ffmpeg::make_av_frame(data, frame.pixel_format_desc());

		filter_.push(av_frame);

		auto av_frames = filter_.poll_all();

		std::vector<core::const_frame> frames;

		BOOST_FOREACH(auto av_frame, av_frames)
			frames.push_back(ffmpeg::make_frame(frame.stream_tag(), av_frame, frame.frame_rate(), frame_factory));
		
		return frames;
	}		
};

deinterlacer::deinterlacer() : impl_(new impl()){}
deinterlacer::~deinterlacer(){}
std::vector<core::const_frame> deinterlacer::operator()(const core::const_frame& frame, core::frame_factory& frame_factory){return (*impl_)(frame, frame_factory);}

}}}