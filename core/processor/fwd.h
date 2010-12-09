#pragma once

#include <memory>

namespace caspar { namespace core {
		
class gpu_frame;
typedef std::shared_ptr<gpu_frame> gpu_frame_ptr;

class write_frame;
typedef std::shared_ptr<write_frame> write_frame_ptr;

class transform_frame;
typedef std::shared_ptr<transform_frame> transform_frame_ptr;

class composite_frame;
typedef std::shared_ptr<composite_frame> composite_frame_ptr;

class consumer_frame;

class frame_shader;
typedef std::shared_ptr<frame_shader> frame_shader_ptr;

class frame_renderer;
typedef std::shared_ptr<frame_renderer> frame_renderer_ptr;

class frame_processor_device;
typedef std::shared_ptr<frame_processor_device> frame_processor_device_ptr;
}}