#pragma once

#include <memory>

namespace caspar::accelerator::ogl {

class device_context final
{
  public:
    device_context();
    ~device_context();

    device_context(const device_context&) = delete;

    device_context& operator=(const device_context&) = delete;

    void postinit();

    void bind();
    void unbind();

  private:
    struct impl;
    std::shared_ptr<impl> impl_;
};

} // namespace caspar::accelerator::ogl
