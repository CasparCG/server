#pragma once

#include <memory>

namespace caspar { namespace renderer {

class render_device;
typedef std::shared_ptr<render_device> render_device_ptr;
typedef std::unique_ptr<render_device> render_device_uptr;
class layer;
typedef std::shared_ptr<layer> layer_ptr;
typedef std::unique_ptr<layer> layer_uptr;

}}
