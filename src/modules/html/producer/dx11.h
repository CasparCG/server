#pragma once

#undef NOMINMAX
#define NOMINMAX

#include <d3d11_1.h>

#include <memory>
#include <string>

namespace caspar { namespace html {

class dx11_context
{
  public:
    dx11_context(ID3D11DeviceContext* ctx);

    dx11_context(const dx11_context&) = delete;
    dx11_context& operator=(const dx11_context&) = delete;

    ID3D11DeviceContext* context() const { return ctx_.get(); }

  private:
    std::shared_ptr<ID3D11DeviceContext> const ctx_;
};

class dx11_device
{
  public:
    dx11_device();
    ~dx11_device();

    dx11_device(const dx11_device&) = delete;
    dx11_device& operator=(const dx11_device&) = delete;

    std::wstring adapter_name() const;

    ID3D11Device* device() const;

    std::shared_ptr<dx11_context> immedidate_context();

    std::shared_ptr<class dx11_texture2d> open_shared_texture(void* handle);

    std::shared_ptr<class dx11_texture2d> create_texture(int width, int height, DXGI_FORMAT format);

    void copy_texture(ID3D11Texture2D*                             dst,
                      uint32_t                                     dst_x,
                      uint32_t                                     dst_y,
                      const std::shared_ptr<class dx11_texture2d>& src,
                      uint32_t                                     src_x,
                      uint32_t                                     src_y,
                      uint32_t                                     src_w,
                      uint32_t                                     src_h);

    static const std::shared_ptr<dx11_device>& get_device();

  private:
    struct impl;
    std::unique_ptr<impl> impl_;
};

class dx11_texture2d final
{
  public:
    dx11_texture2d(ID3D11Texture2D* tex);

    uint32_t width() const { return width_; }

    uint32_t height() const { return height_; }

    DXGI_FORMAT format() const { return format_; }

    void* share_handle() const { return share_handle_; }

    ID3D11Texture2D* texture() const { return texture_.get(); }

  private:
    HANDLE share_handle_;

    std::shared_ptr<ID3D11Texture2D> const texture_;
    uint32_t                               width_  = 0;
    uint32_t                               height_ = 0;
    DXGI_FORMAT                            format_ = DXGI_FORMAT::DXGI_FORMAT_UNKNOWN;
};
}} // namespace caspar::html
