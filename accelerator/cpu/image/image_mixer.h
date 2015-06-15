#pragma once

#include <common/forward.h>
#include <common/memory.h>

#include <core/mixer/image/blend_modes.h>
#include <core/mixer/image/image_mixer.h>
#include <core/fwd.h>

#include <core/frame/frame.h>
#include <core/frame/frame_visitor.h>
#include <core/video_format.h>

namespace caspar { namespace accelerator { namespace cpu {
	
typedef cache_aligned_vector<uint8_t> buffer;

class image_mixer final : public core::image_mixer
{
public:

	// Static Members

	// Constructors

	image_mixer();
	~image_mixer();

	// Methods	

	virtual void push(const core::frame_transform& frame);
	virtual void visit(const core::const_frame& frame);
	virtual void pop();
		
	std::future<array<const std::uint8_t>> operator()(const core::video_format_desc& format_desc) override;
		
	core::mutable_frame create_frame(const void* tag, const core::pixel_format_desc& desc) override;

	// Properties

private:
	struct impl;
	spl::unique_ptr<impl> impl_;
};

}}}