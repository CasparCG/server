#include "../StdAfx.h"

#include "frame_processor_device.h"

#include "frame_renderer.h"
#include "write_frame.h"
#include "draw_frame.h"
#include "read_frame.h"

#include "../format/video_format.h"

#include "../../common/exception/exceptions.h"
#include "../../common/concurrency/executor.h"
#include "../../common/gl/utility.h"

#include <tbb/concurrent_queue.h>
#include <tbb/concurrent_unordered_map.h>
#include <tbb/concurrent_vector.h>

#include <boost/range/algorithm.hpp>
#include <boost/thread.hpp>

#include <functional>

namespace caspar { namespace core {
	
struct frame_processor_device::implementation : boost::noncopyable
{	
	implementation(const video_format_desc& format_desc) : fmt_(format_desc), underrun_count_(0)
	{		
		output_.set_capacity(3);
		executor_.start();
		executor_.invoke([=]
		{
			renderer_.reset(new frame_renderer(format_desc));
		});
	}
				
	void send(safe_ptr<draw_frame>&& frame)
	{			
		auto future = executor_.begin_invoke([=]() -> safe_ptr<const read_frame>
		{
			return renderer_->render(safe_ptr<draw_frame>(frame));
		});	
		output_.push(std::move(future)); // Blocks
	}

	safe_ptr<const read_frame> receive()
	{
		boost::shared_future<safe_ptr<const read_frame>> future;

		if(!output_.try_pop(future))
		{
			if(underrun_count_++ == 0)			
				CASPAR_LOG(trace) << "### Frame Processor Output Underrun has STARTED. ###";
			
			output_.pop(future);
		}	
		else if(underrun_count_ > 0)
		{
			CASPAR_LOG(trace) << "### Frame Processor Output Underrun has ENDED with " << underrun_count_ << " ticks. ###";
			underrun_count_ = 0;
		}

		return future.get();
	}
	
	safe_ptr<write_frame> create_frame(const pixel_format_desc& desc)
	{
		auto pool = &pools_[desc];
		std::shared_ptr<write_frame> frame;
		if(!pool->try_pop(frame))
			frame = executor_.invoke([&]{return std::make_shared<write_frame>(desc);});
		
		frame = std::shared_ptr<write_frame>(frame.get(), [=](write_frame*)
		{
			executor_.begin_invoke([=]
			{
				frame->audio_data().clear();
				pool->push(frame);
			});
		});

		return safe_ptr<write_frame>(frame);
	}
				
	executor executor_;	

	std::unique_ptr<frame_renderer> renderer_;	
				
	tbb::concurrent_bounded_queue<boost::shared_future<safe_ptr<const read_frame>>> output_;	

	tbb::concurrent_unordered_map<pixel_format_desc, tbb::concurrent_bounded_queue<std::shared_ptr<write_frame>>, std::hash<pixel_format_desc>> pools_;
	
	const video_format_desc fmt_;
	long underrun_count_;
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