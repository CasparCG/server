#pragma once

#include <common/forward.h>
#include <common/memory.h>

#include <core/mixer/image/blend_modes.h>
#include <core/mixer/image/image_mixer.h>

#include <core/frame/frame.h>
#include <core/frame/frame_visitor.h>
#include <core/video_format.h>

FORWARD1(boost, template<typename> class shared_future);
FORWARD1(boost, template<typename> class iterator_range);
FORWARD2(caspar, core, class frame);
FORWARD2(caspar, core, struct pixel_format_desc);
FORWARD2(caspar, core, struct video_format_desc);
FORWARD2(caspar, core, class mutable_frame);
FORWARD2(caspar, core, struct frame_transform);

namespace caspar { namespace accelerator { namespace cpu {
	
typedef std::vector<uint8_t, tbb::cache_aligned_allocator<uint8_t>> buffer;

class image_mixer sealed : public core::image_mixer
{
public:

	// Static Members

	// Constructors

	image_mixer();
	~image_mixer();

	// Methods	

	virtual void begin_layer(core::blend_mode blend_mode);
	virtual void end_layer();

	virtual void push(const core::frame_transform& frame);
	virtual void visit(const core::const_frame& frame);
	virtual void pop();
		
	boost::unique_future<array<const std::uint8_t>> operator()(const core::video_format_desc& format_desc) override;
		
	core::mutable_frame create_frame(const void* tag, const core::pixel_format_desc& desc) override;

	// Properties

private:
	struct impl;
	spl::unique_ptr<impl> impl_;
};

}}}