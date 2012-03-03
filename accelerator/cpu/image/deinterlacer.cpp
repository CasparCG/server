#include "../../StdAfx.h"

#include "deinterlacer.h"
/*
#include <core/frame/frame_factory.h>

#include <modules/ffmpeg/producer/filter/filter.h>
#include <modules/ffmpeg/producer/util/util.h>

#include <tbb/concurrent_hash_map.h>

#include <tuple>

namespace caspar { namespace accelerator { namespace cpu {

struct deinterlacer::impl : public std::enable_shared_from_this<impl> 
{
	ffmpeg::filter filter_;

	typedef tbb::concurrent_hash_map<const void*, std::tuple<boost::signals2::scoped_connection, std::vector<core::const_frame>>> cache_t; 

	cache_t frame_cache_;

public:

	impl() 
		: filter_(L"YADIF=1:-1")
	{
	}

	std::vector<core::const_frame> operator()(const core::const_frame& frame, core::frame_factory& frame_factory)
	{
		auto tag = frame.data_tag();

		cache_t::const_accessor a;
		
		if(frame_cache_.find(a, tag))
			return std::get<1>(a->second);
		
		std::array<uint8_t*, 4> data = {};
		for(int n = 0; n < frame.pixel_format_desc().planes.size(); ++n)
			data[n] = const_cast<uint8_t*>(frame.image_data(n).begin());

		auto av_frame = ffmpeg::make_av_frame(data, frame.pixel_format_desc());

		filter_.push(av_frame);

		auto av_frames = filter_.poll_all();

		std::vector<core::const_frame> frames;

		BOOST_FOREACH(auto av_frame, av_frames)
			frames.push_back(ffmpeg::make_frame(tag, av_frame, frame.frame_rate(), frame_factory, 0));

		std::weak_ptr<impl> self = shared_from_this();
		auto connection = frame.on_released.connect(std::function<void()>([self, tag]() mutable
		{
			auto self2 = self.lock();
			if(self2)
				self2->frame_cache_.erase(tag);
		}));

		frame_cache_.insert(std::make_pair(&frame, std::make_tuple(connection, frames)));

		return frames;
	}		
};

deinterlacer::deinterlacer() : impl_(new impl()){}
deinterlacer::~deinterlacer(){}
std::vector<core::const_frame> deinterlacer::operator()(const core::const_frame& frame, core::frame_factory& frame_factory){return (*impl_)(frame, frame_factory);}

}}}*/