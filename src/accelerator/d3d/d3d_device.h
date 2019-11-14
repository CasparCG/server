#pragma once

#include <memory>
#include <string>

namespace caspar { namespace accelerator { namespace d3d {
class d3d_device
{
  public:
    d3d_device();
    ~d3d_device();

    d3d_device(const d3d_device&) = delete;
    d3d_device& operator=(const d3d_device&) = delete;

    std::wstring adapter_name() const;

    void* device() const;

    std::shared_ptr<class d3d_device_context> immedidate_context();

    std::shared_ptr<class d3d_texture2d> open_shared_texture(void* handle);

    static const std::shared_ptr<d3d_device>& get_device();

  private:
    struct impl;
    std::shared_ptr<impl> impl_;
};
}}} // namespace caspar::accelerator::d3d
