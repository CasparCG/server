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

#include <Glee.h>
#include <SFML/Window.hpp>

#include <tbb/concurrent_queue.h>
#include <tbb/concurrent_unordered_map.h>
#include <tbb/concurrent_vector.h>

#include <boost/range/algorithm.hpp>
#include <boost/thread.hpp>

#include <functional>

namespace caspar { namespace core {
	
using namespace common::gl;

struct frame_processor_device::implementation : boost::noncopyable
{	
	typedef tbb::concurrent_bounded_queue<pbo_ptr> pbo_pool;

	implementation(const video_format_desc& format_desc) : fmt_(format_desc), underrun_count_(0)
	{		
		output_.set_capacity(3);
		executor_.start();
		executor_.invoke([=]
		{
			ogl_context_.reset(new sf::Context());
			ogl_context_->SetActive(true);

			renderer_.reset(new frame_renderer(format_desc));
		});
	}
				
	void send(const draw_frame& frame)
	{			
		auto future = executor_.begin_invoke([=]{return renderer_->render(frame);});	
		output_.push(std::move(future)); // Blocks
	}

	read_frame receive()
	{
		boost::shared_future<read_frame> future;

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
	
	write_frame create_frame(const pixel_format_desc& desc)
	{
		std::vector<pbo_pool*> pools(desc.planes.size());
		boost::range::transform(desc.planes, pools.begin(), [&](const pixel_format_desc::plane& plane)
		{
			return &pbo_pools_[((plane.width & 0x7FF)) << 0 | ((plane.height & 0x7FF)) << 11 | plane.channels];
		});

		bool filled = true;
		std::vector<pbo_ptr> pbos(pools.size());
		boost::range::transform(pools, pbos.begin(), [&](pbo_pool* pool) -> pbo_ptr
		{
			pbo_ptr pbo;
			filled &= pool->try_pop(pbo);
			return pbo;
		});
		
		if(!filled)
		{
			executor_.invoke([&]
			{
				for(size_t n = 0; n < pbos.size(); ++n)
				{
					if(!pbos[n])
						pbos[n] = create_pooled_pbo(desc.planes[n], pools[n]);
				}
			});
		}
		
		return write_frame(std::move(pbos), desc);
	}

	pbo_ptr create_pooled_pbo(const pixel_format_desc::plane& plane, pbo_pool* pool)
	{
		static GLenum mapping[] = {GL_LUMINANCE, GL_LUMINANCE_ALPHA, GL_BGR, GL_BGRA};
		auto pooled_pbo = std::make_shared<pbo>(plane.width, plane.height, mapping[plane.channels-1]);
		pooled_pbo->end_write();
		return pbo_ptr(pooled_pbo.get(), [=](pbo*)
		{
			executor_.begin_invoke([=]
			{
				pooled_pbo->end_write();
				pool->push(pooled_pbo);
			});
		});
	}
				
	common::executor executor_;	

	std::unique_ptr<sf::Context> ogl_context_;
	std::unique_ptr<frame_renderer> renderer_;	
				
	tbb::concurrent_bounded_queue<boost::shared_future<read_frame>> output_;	

	tbb::concurrent_unordered_map<int, pbo_pool> pbo_pools_;
	
	const video_format_desc fmt_;
	long underrun_count_;
};
	
frame_processor_device::frame_processor_device(const video_format_desc& format_desc) : impl_(new implementation(format_desc)){}
void frame_processor_device::send(const draw_frame& frame){impl_->send(std::move(frame));}
read_frame frame_processor_device::receive(){return impl_->receive();}
const video_format_desc& frame_processor_device::get_video_format_desc() const { return impl_->fmt_; }

write_frame frame_processor_device::create_frame(const pixel_format_desc& desc){ return impl_->create_frame(desc); }		
write_frame frame_processor_device::create_frame(size_t width, size_t height)
{
	// Create bgra frame
	pixel_format_desc desc;
	desc.pix_fmt = pixel_format::bgra;
	desc.planes.push_back(pixel_format_desc::plane(width, height, 4));
	return create_frame(desc);
}
			
write_frame frame_processor_device::create_frame()
{
	// Create bgra frame with output resolution
	pixel_format_desc desc;
	desc.pix_fmt = pixel_format::bgra;
	desc.planes.push_back(pixel_format_desc::plane(get_video_format_desc().width, get_video_format_desc().height, 4));
	return create_frame(desc);
}

}}