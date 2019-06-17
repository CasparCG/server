#pragma once

#undef NOMINMAX
#define NOMINMAX

#include <d3d11_1.h>

#include <memory>

namespace caspar { namespace accelerator { namespace d3d {
class d3d_device_context
{
  public:
    d3d_device_context(ID3D11DeviceContext* ctx);

    d3d_device_context(const d3d_device_context&) = delete;
    d3d_device_context& operator=(const d3d_device_context&) = delete;

    ID3D11DeviceContext* context() const { return ctx_.get(); }

  private:
    std::shared_ptr<ID3D11DeviceContext> const ctx_;
};
}}} // namespace caspar::accelerator::d3d
