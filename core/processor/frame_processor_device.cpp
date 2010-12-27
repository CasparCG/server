#include "../StdAfx.h"

#include "frame_processor_device.h"

#include "audio_processor.h"
#include "image_processor.h"

#include "write_frame.h"
#include "read_frame.h"

#include "../format/video_format.h"

#include <common/exception/exceptions.h>
#include <common/concurrency/executor.h>
#include <common/gl/utility.h>

#include <tbb/concurrent_queue.h>
#include <tbb/concurrent_unordered_map.h>

#include <boost/range/algorithm.hpp>

namespace caspar { namespace core {
	
struct frame_processor_device::implementation : boost::noncopyable
{	
	implementation(const video_format_desc& format_desc) : fmt_(format_desc), image_processor_(format_desc){}
			
	safe_ptr<const read_frame> process(safe_ptr<draw_frame>&& frame)
	{			
		auto image = image_processor_.begin_pass();
		frame->process_image(image_processor_);
		image_processor_.end_pass();

		auto audio = audio_processor_.begin_pass();
		frame->process_audio(audio_processor_);
		audio_processor_.end_pass();

		return make_safe<const read_frame>(std::move(image.get()), std::move(audio));
	}
		
	safe_ptr<write_frame> create_frame(const pixel_format_desc& desc)
	{
		return make_safe<write_frame>(desc, image_processor_.create_buffers(desc));
	}
	
	const video_format_desc format_desc_;
		
	audio_processor	audio_processor_;
	image_processor image_processor_;
											
	const video_format_desc fmt_;
};
	
frame_processor_device::frame_processor_device(frame_processor_device&& other) : impl_(std::move(other.impl_)){}
frame_processor_device::frame_processor_device(const video_format_desc& format_desc) : impl_(new implementation(format_desc)){}
safe_ptr<const read_frame> frame_processor_device::process(safe_ptr<draw_frame>&& frame){return impl_->process(std::move(frame));}
const video_format_desc& frame_processor_device::get_video_format_desc() const { return impl_->fmt_; }
safe_ptr<write_frame> frame_processor_device::create_frame(const pixel_format_desc& desc){ return impl_->create_frame(desc); }		
safe_ptr<write_frame> frame_processor_device::create_frame(size_t width, size_t height, pixel_format::type pix_fmt)
{
	// Create bgra frame
	pixel_format_desc desc;
	desc.pix_fmt = pix_fmt;
	desc.planes.push_back(pixel_format_desc::plane(width, height, 4));
	return create_frame(desc);
}
			
safe_ptr<write_frame> frame_processor_device::create_frame(pixel_format::type pix_fmt)
{
	// Create bgra frame with output resolution
	pixel_format_desc desc;
	desc.pix_fmt = pix_fmt;
	desc.planes.push_back(pixel_format_desc::plane(get_video_format_desc().width, get_video_format_desc().height, 4));
	return create_frame(desc);
}

}}