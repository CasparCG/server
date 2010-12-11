#pragma once

#include <memory>

namespace caspar { namespace core {
	
class read_frame;
class write_frame;
class draw_frame;

struct draw_frame_impl;
typedef std::shared_ptr<draw_frame_impl> draw_frame_impl_ptr;

class transform_frame;
typedef std::shared_ptr<transform_frame> transform_frame_ptr;

class composite_frame;
typedef std::shared_ptr<composite_frame> composite_frame_ptr;

class frame_shader;
typedef std::shared_ptr<frame_shader> frame_shader_ptr;

class frame_renderer;
typedef std::shared_ptr<frame_renderer> frame_renderer_ptr;

class frame_processor_device;
typedef std::shared_ptr<frame_processor_device> frame_processor_device_ptr;
}}