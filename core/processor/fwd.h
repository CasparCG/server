#pragma once

#include <memory>

namespace caspar { namespace core {
	
class read_frame;
class write_frame;
class draw_frame;

class draw_frame;

class transform_frame;
class composite_frame;

class frame_shader;
typedef std::shared_ptr<frame_shader> frame_shader_ptr;

class frame_renderer;
typedef std::shared_ptr<frame_renderer> frame_renderer_ptr;

class frame_processor_device;
typedef std::shared_ptr<frame_processor_device> frame_processor_device_ptr;
}}