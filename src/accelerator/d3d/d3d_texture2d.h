#pragma once

#undef NOMINMAX
#define NOMINMAX

#include <d3d11_1.h>

#include <memory>

namespace caspar { namespace accelerator { namespace d3d {

class d3d_texture2d
{
  public:
    d3d_texture2d(ID3D11Texture2D* tex);

    ~d3d_texture2d();

    d3d_texture2d(const d3d_texture2d&)            = delete;
    d3d_texture2d& operator=(const d3d_texture2d&) = delete;

    uint32_t width() const { return width_; }

    uint32_t height() const { return height_; }

    DXGI_FORMAT format() const { return format_; }

    void* share_handle() const { return share_handle_; }

    ID3D11Texture2D* texture() const { return texture_; }

    uint32_t gl_texture_id() const { return gl_texture_id_; }

    void gen_gl_texture(std::shared_ptr<ogl::device>);

    void lock_gl();

    void unlock_gl();

  private:
    HANDLE share_handle_;

    ID3D11Texture2D* texture_;
    uint32_t         width_  = 0;
    uint32_t         height_ = 0;
    DXGI_FORMAT      format_ = DXGI_FORMAT::DXGI_FORMAT_UNKNOWN;

    std::weak_ptr<ogl::device> ogl_;
    HANDLE                     texture_handle_ = nullptr;
    uint32_t                   gl_texture_id_  = 0;
    bool                       is_locked_ = false;
};
}}} // namespace caspar::accelerator::d3d
