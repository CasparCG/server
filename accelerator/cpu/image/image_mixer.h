#pragma once

#include <common/forward.h>
#include <common/spl/memory.h>

#include <core/mixer/image/blend_modes.h>
#include <core/mixer/image/image_mixer.h>

#include <core/frame/frame_visitor.h>

FORWARD1(boost, template<typename> class shared_future);
FORWARD1(boost, template<typename> class iterator_range);
FORWARD2(caspar, core, struct write_frame);
FORWARD2(caspar, core, struct pixel_format_desc);
FORWARD2(caspar, core, struct video_format_desc);
FORWARD2(caspar, core, struct data_Frame);
FORWARD2(caspar, core, struct frame_transform);

namespace caspar { namespace accelerator { namespace cpu {
	
class image_mixer sealed : public core::image_mixer
{
public:
	image_mixer();
	
	virtual void push(core::frame_transform& frame);
	virtual void visit(core::data_frame& frame);
	virtual void pop();

	void begin_layer(core::blend_mode blend_mode);
	void end_layer();
		
	// NOTE: Content of return future is only valid while future is valid.
	virtual ::boost::shared_future<::boost::iterator_range<const uint8_t*>> operator()(const core::video_format_desc& format_desc) override;
		
	virtual spl::shared_ptr<core::write_frame> create_frame(const void* tag, const core::pixel_format_desc& desc) override;
private:
	struct impl;
	spl::shared_ptr<impl> impl_;
};

}}}