#pragma once

#include <d3d11_1.h>

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

    ID3D11Device* device() const;

    std::shared_ptr<class d3d_device_context> immedidate_context();

    std::shared_ptr<class d3d_texture2d> open_shared_texture(void* handle);

    std::shared_ptr<class d3d_texture2d> create_texture(int width, int height, DXGI_FORMAT format);

    void copy_texture(ID3D11Texture2D*                            dst,
                      uint32_t                                    dst_x,
                      uint32_t                                    dst_y,
                      const std::shared_ptr<class d3d_texture2d>& src,
                      uint32_t                                    src_x,
                      uint32_t                                    src_y,
                      uint32_t                                    src_w,
                      uint32_t                                    src_h);

    static const std::shared_ptr<d3d_device>& get_device();

  private:
    struct impl;
    std::shared_ptr<impl> impl_;
};
}}} // namespace caspar::accelerator::d3d
