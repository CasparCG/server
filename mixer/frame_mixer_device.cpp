#include "StdAfx.h"

#include "frame_mixer_device.h"

#include "audio/audio_mixer.h"
#include "audio/audio_transform.h"
#include "frame/write_frame.h"
#include "frame/read_frame.h"
#include "image/image_mixer.h"
#include "image/image_transform.h"

#include <common/exception/exceptions.h>
#include <common/concurrency/executor.h>
#include <common/diagnostics/graph.h>
#include <common/utility/timer.h>
#include <common/gl/gl_check.h>

#include <core/video_format.h>

#include <tbb/concurrent_queue.h>
#include <tbb/concurrent_unordered_map.h>

#include <boost/range/algorithm.hpp>

#include <unordered_map>

namespace caspar { namespace core {
	
struct frame_mixer_device::implementation : boost::noncopyable
{		
	safe_ptr<diagnostics::graph> graph_;
	timer perf_timer_;
	timer wait_perf_timer_;

	const video_format_desc format_desc_;

	audio_mixer	audio_mixer_;
	image_mixer image_mixer_;

	output_func output_;

	std::unordered_map<int, image_transform> image_transforms_;
	std::unordered_map<int, audio_transform> audio_transforms_;

	executor executor_;
public:
	implementation(const video_format_desc& format_desc, const output_func& output) 
		: graph_(diagnostics::create_graph("mixer"))
		, format_desc_(format_desc)
		, image_mixer_(format_desc)
		, output_(output)
		, executor_(L"frame_mixer_device")
	{
		graph_->guide("frame-time", 0.5f);	
		graph_->set_color("frame-time", diagnostics::color(1.0f, 0.0f, 0.0f));
		graph_->set_color("frame-wait", diagnostics::color(0.1f, 0.7f, 0.8f));
		graph_->set_color("output-buffer", diagnostics::color( 0.0f, 1.0f, 0.0f));		
		executor_.start();
		executor_.set_capacity(2);
	}
		
	~implementation()
	{
		CASPAR_LOG(info) << "Shutting down mixer-device.";
	}

	void send(const std::vector<safe_ptr<draw_frame>>& frames)
	{			
		executor_.begin_invoke([=]
		{
			perf_timer_.reset();
			auto image = image_mixer_.begin_pass();
			BOOST_FOREACH(auto& frame, frames)
			{
				image_mixer_.begin(image_transforms_[frame->get_layer_index()]);
				frame->process_image(image_mixer_);
				image_mixer_.end();
			}
			image_mixer_.end_pass();

			auto audio = audio_mixer_.begin_pass();
			BOOST_FOREACH(auto& frame, frames)
			{
				audio_mixer_.begin(audio_transforms_[frame->get_layer_index()]);
				frame->process_audio(audio_mixer_);
				audio_mixer_.end();
			}
			audio_mixer_.end_pass();
			graph_->update("frame-time", static_cast<float>(perf_timer_.elapsed()/format_desc_.interval*0.5));

			wait_perf_timer_.reset();
			output_(make_safe<const read_frame>(std::move(image.get()), std::move(audio)));
			graph_->update("frame-wait", static_cast<float>(wait_perf_timer_.elapsed()/format_desc_.interval*0.5));
		});
		graph_->update("output-buffer", static_cast<float>(executor_.size())/static_cast<float>(executor_.capacity()));
	}
		
	safe_ptr<write_frame> create_frame(const pixel_format_desc& desc)
	{
		return make_safe<write_frame>(desc, image_mixer_.create_buffers(desc));
	}
		
	image_transform get_image_transform(int index)
	{
		return executor_.invoke([&]{return image_transforms_[index];});
	}

	audio_transform get_audio_transform(int index)
	{
		return executor_.invoke([&]{return audio_transforms_[index];});
	}

	void set_image_transform(int index, image_transform&& transform)
	{
		return executor_.invoke([&]{image_transforms_[index] = std::move(transform);});
	}

	void set_audio_transform(int index, audio_transform&& transform)
	{
		return executor_.invoke([&]{audio_transforms_[index] = std::move(transform);});
	}
};
	
frame_mixer_device::frame_mixer_device(const video_format_desc& format_desc, const output_func& output) : impl_(new implementation(format_desc, output)){}
frame_mixer_device::frame_mixer_device(frame_mixer_device&& other) : impl_(std::move(other.impl_)){}
void frame_mixer_device::send(const std::vector<safe_ptr<draw_frame>>& frames){impl_->send(frames);}
const video_format_desc& frame_mixer_device::get_video_format_desc() const { return impl_->format_desc_; }
safe_ptr<write_frame> frame_mixer_device::create_frame(const pixel_format_desc& desc){ return impl_->create_frame(desc); }		
safe_ptr<write_frame> frame_mixer_device::create_frame(size_t width, size_t height, pixel_format::type pix_fmt)
{
	// Create bgra frame
	pixel_format_desc desc;
	desc.pix_fmt = pix_fmt;
	desc.planes.push_back(pixel_format_desc::plane(width, height, 4));
	return create_frame(desc);
}
			
safe_ptr<write_frame> frame_mixer_device::create_frame(pixel_format::type pix_fmt)
{
	// Create bgra frame with output resolution
	pixel_format_desc desc;
	desc.pix_fmt = pix_fmt;
	desc.planes.push_back(pixel_format_desc::plane(get_video_format_desc().width, get_video_format_desc().height, 4));
	return create_frame(desc);
}
image_transform frame_mixer_device::get_image_transform(int index){return impl_->get_image_transform(index);}
audio_transform frame_mixer_device::get_audio_transform(int index){return impl_->get_audio_transform(index);}
void frame_mixer_device::set_image_transform(int index, image_transform&& transform){impl_->set_image_transform(index, std::move(transform));}
void frame_mixer_device::set_audio_transform(int index, audio_transform&& transform){impl_->set_audio_transform(index, std::move(transform));}

}}