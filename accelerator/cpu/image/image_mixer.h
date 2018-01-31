#pragma once

#include <core/mixer/image/blend_modes.h>
#include <core/mixer/image/image_mixer.h>

#include <core/frame/frame.h>
#include <core/frame/frame_visitor.h>
#include <core/video_format.h>

#include <memory>

namespace caspar { namespace accelerator { namespace cpu {

class image_mixer final : public core::image_mixer
{
public:

	// Static Members

	// Constructors

	image_mixer(int channel_id);
	~image_mixer();

	// Methods

	virtual void push(const core::frame_transform& frame);
	virtual void visit(const core::const_frame& frame);
	virtual void pop();

	std::future<array<const std::uint8_t>> operator()(const core::video_format_desc& format_desc, bool straighten_alpha) override;

	core::mutable_frame create_frame(const void* tag, const core::pixel_format_desc& desc) override;

	// Properties
private:
	struct impl;
	std::unique_ptr<impl> impl_;
};

}}}