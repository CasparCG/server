#include "../StdAfx.h"

#include "frame_processor_device.h"

#include "image_processor.h"
#include "write_frame.h"
#include "read_frame.h"

#include "../format/video_format.h"

#include "../../common/exception/exceptions.h"
#include "../../common/concurrency/executor.h"
#include "../../common/gl/utility.h"

#include <tbb/concurrent_queue.h>
#include <tbb/concurrent_unordered_map.h>

#include <boost/range/algorithm.hpp>
#include <boost/thread.hpp>

#include <functional>

namespace caspar { namespace core {
	
struct frame_processor_device::implementation : boost::noncopyable
{	
	typedef boost::shared_future<safe_ptr<const read_frame>> output_type;

	implementation(const video_format_desc& format_desc) : fmt_(format_desc)
	{		
		output_.set_capacity(3);
		executor_.start();
		executor_.invoke([=]{image_processor_.reset(new image_processor(format_desc));});
	}
				
	void send(safe_ptr<draw_frame>&& frame)
	{			
		output_.push(executor_.begin_invoke([=]{return image_processor_->render(safe_ptr<draw_frame>(frame));})); // Blocks
	}

	safe_ptr<const read_frame> receive()
	{
		output_type future;
		output_.pop(future);
		return future.get();
	}
	
	safe_ptr<write_frame> create_frame(const pixel_format_desc& desc)
	{
		auto pool = &pools_[desc];
		std::shared_ptr<write_frame> frame;
		if(!pool->try_pop(frame))
			frame = executor_.invoke([&]{return std::make_shared<write_frame>(desc);});		
		return safe_ptr<write_frame>(frame.get(), [=](write_frame*)
		{
			executor_.begin_invoke([=]{frame->map();});
			frame->audio_data().clear();
			pool->push(frame);
		});
	}
				
	executor executor_;	

	std::unique_ptr<image_processor> image_processor_;	
				
	tbb::concurrent_bounded_queue<output_type> output_;	
	tbb::concurrent_unordered_map<pixel_format_desc, tbb::concurrent_bounded_queue<std::shared_ptr<write_frame>>, std::hash<pixel_format_desc>> pools_;
	
	const video_format_desc fmt_;
};
	
frame_processor_device::frame_processor_device(frame_processor_device&& other) : impl_(std::move(other.impl_)){}
frame_processor_device::frame_processor_device(const video_format_desc& format_desc) : impl_(new implementation(format_desc)){}
void frame_processor_device::send(safe_ptr<draw_frame>&& frame){impl_->send(std::move(frame));}
safe_ptr<const read_frame> frame_processor_device::receive(){return impl_->receive();}
const video_format_desc& frame_processor_device::get_video_format_desc() const { return impl_->fmt_; }
safe_ptr<write_frame> frame_processor_device::create_frame(const pixel_format_desc& desc){ return impl_->create_frame(desc); }		
safe_ptr<write_frame> frame_processor_device::create_frame(size_t width, size_t height)
{
	// Create bgra frame
	pixel_format_desc desc;
	desc.pix_fmt = pixel_format::bgra;
	desc.planes.push_back(pixel_format_desc::plane(width, height, 4));
	return create_frame(desc);
}
			
safe_ptr<write_frame> frame_processor_device::create_frame()
{
	// Create bgra frame with output resolution
	pixel_format_desc desc;
	desc.pix_fmt = pixel_format::bgra;
	desc.planes.push_back(pixel_format_desc::plane(get_video_format_desc().width, get_video_format_desc().height, 4));
	return create_frame(desc);
}

}}