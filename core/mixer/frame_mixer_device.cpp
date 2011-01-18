#include "../StdAfx.h"

#include "frame_mixer_device.h"

#include "audio/audio_mixer.h"
#include "image/image_mixer.h"

#include "frame/write_frame.h"
#include "frame/read_frame.h"

#include "../video_format.h"

#include <common/exception/exceptions.h>
#include <common/concurrency/executor.h>
#include <common/gl/gl_check.h>

#include <tbb/concurrent_queue.h>
#include <tbb/concurrent_unordered_map.h>

#include <boost/range/algorithm.hpp>

#include <unordered_map>

namespace caspar { namespace core {
	
struct frame_mixer_device::implementation : boost::noncopyable
{		
	const video_format_desc format_desc_;

	audio_mixer	audio_mixer_;
	image_mixer image_mixer_;

	output_func output_;

	executor executor_;

	std::unordered_map<int, image_transform> image_transforms_;
	std::unordered_map<int, audio_transform> audio_transforms_;

public:
	implementation(const video_format_desc& format_desc, const output_func& output) 
		: format_desc_(format_desc)
		, image_mixer_(format_desc)
		, output_(output)
	{
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

			output_(make_safe<const read_frame>(std::move(image.get()), std::move(audio)));
		});
	}
		
	safe_ptr<write_frame> create_frame(const pixel_format_desc& desc)
	{
		return make_safe<write_frame>(desc, image_mixer_.create_buffers(desc));
	}

	image_transform& image(int index) 
	{
		return image_transforms_[index];
	}

	audio_transform& audio(int index) 
	{
		return audio_transforms_[index];
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

image_transform& frame_mixer_device::image(int index) { return impl_->image(index);}
audio_transform& frame_mixer_device::audio(int index) { return impl_->audio(index);}

}}