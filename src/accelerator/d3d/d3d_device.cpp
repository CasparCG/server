#include "d3d_device.h"

#include "d3d_device_context.h"
#include "d3d_texture2d.h"

#include <common/except.h>
#include <common/log.h>
#include <common/utf.h>

namespace caspar { namespace accelerator { namespace d3d {
struct d3d_device::impl
{
    std::wstring                        adaptor_name_ = L"N/A";
    std::shared_ptr<ID3D11Device>       device_;
    std::shared_ptr<d3d_device_context> ctx_;

    impl()
    {
        HRESULT hr;

        UINT flags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;

        D3D_FEATURE_LEVEL feature_levels[] = {
            D3D_FEATURE_LEVEL_11_1,
            D3D_FEATURE_LEVEL_11_0,
            D3D_FEATURE_LEVEL_10_1,
            D3D_FEATURE_LEVEL_10_0,
            D3D_FEATURE_LEVEL_9_3,
        };
        UINT num_feature_levels = sizeof(feature_levels) / sizeof(feature_levels[0]);

        D3D_FEATURE_LEVEL selected_level = D3D_FEATURE_LEVEL_9_3;

        ID3D11Device*        pdev = nullptr;
        ID3D11DeviceContext* pctx = nullptr;

        hr = D3D11CreateDevice(nullptr,
                               D3D_DRIVER_TYPE_HARDWARE,
                               nullptr,
                               flags,
                               feature_levels,
                               num_feature_levels,
                               D3D11_SDK_VERSION,
                               &pdev,
                               &selected_level,
                               &pctx);

        if (hr == E_INVALIDARG) {
            // DirectX 11.0 platforms will not recognize D3D_FEATURE_LEVEL_11_1
            // so we need to retry without it
            hr = D3D11CreateDevice(nullptr,
                                   D3D_DRIVER_TYPE_HARDWARE,
                                   nullptr,
                                   flags,
                                   &feature_levels[1],
                                   num_feature_levels - 1,
                                   D3D11_SDK_VERSION,
                                   &pdev,
                                   &selected_level,
                                   &pctx);
        }

        if (SUCCEEDED(hr)) {
            device_ = std::shared_ptr<ID3D11Device>(pdev, [](ID3D11Device* p) {
                if (p)
                    p->Release();
            });
            ctx_    = std::make_shared<d3d_device_context>(pctx);

            {
                IDXGIDevice* dxgi_dev = nullptr;
                hr                    = device_->QueryInterface(__uuidof(dxgi_dev), (void**)&dxgi_dev);
                if (SUCCEEDED(hr)) {
                    IDXGIAdapter* dxgi_adapt = nullptr;
                    hr                       = dxgi_dev->GetAdapter(&dxgi_adapt);
                    dxgi_dev->Release();
                    if (SUCCEEDED(hr)) {
                        DXGI_ADAPTER_DESC desc;
                        hr = dxgi_adapt->GetDesc(&desc);
                        dxgi_adapt->Release();
                        if (SUCCEEDED(hr)) {
                            adaptor_name_ = u16(desc.Description);
                        }
                    }
                }
            }

            CASPAR_LOG(info) << L"D3D11: Selected adapter: " << adaptor_name_;

            CASPAR_LOG(info) << L"D3D11: Selected feature level: " << selected_level;
        } else
            CASPAR_THROW_EXCEPTION(bad_alloc() << msg_info(L"Failed to create d3d11 device"));
    }

    std::shared_ptr<d3d_texture2d> open_shared_texture(void* handle)
    {
        ID3D11Texture2D* tex = nullptr;
        auto             hr  = device_->OpenSharedResource(handle, __uuidof(ID3D11Texture2D), (void**)(&tex));
        if (FAILED(hr))
            return nullptr;

        return std::make_shared<d3d_texture2d>(tex);
    }

    std::shared_ptr<d3d_texture2d> create_texture(int width, int height, DXGI_FORMAT format)
    {
        D3D11_TEXTURE2D_DESC td;
        memset(&td, 0, sizeof(td));

        td.Width            = width;
        td.Height           = height;
        td.MipLevels        = 1;
        td.ArraySize        = 1;
        td.Format           = format;
        td.SampleDesc.Count = 1;
        td.CPUAccessFlags   = D3D11_CPU_ACCESS_READ;
        td.Usage            = D3D11_USAGE_STAGING;

        ID3D11Texture2D* tex = nullptr;
        auto             hr  = device_->CreateTexture2D(&td, nullptr, &tex);
        if (FAILED(hr))
            return nullptr;

        return std::make_shared<d3d_texture2d>(tex);
    }

    void copy_texture(ID3D11Texture2D*                      dst,
                      uint32_t                              dst_x,
                      uint32_t                              dst_y,
                      const std::shared_ptr<d3d_texture2d>& src,
                      uint32_t                              src_x,
                      uint32_t                              src_y,
                      uint32_t                              src_w,
                      uint32_t                              src_h)
    {
        if (dst_x == 0 && dst_y == 0 && src_x == 0 && src_y == 0 && src_w == 0 && src_h == 0) {
            ctx_->context()->CopyResource(dst, src->texture());
        } else {
            D3D11_BOX sbox;

            sbox.left = src_x;
            if (src_w > 0)
                sbox.right = src_x + src_w;
            else
                sbox.right = src->width() - 1;

            sbox.top = src_y;
            if (src_h > 0)
                sbox.bottom = src_y + src_h;
            else
                sbox.bottom = src->height() - 1;

            sbox.front = 0;
            sbox.back  = 1;

            ctx_->context()->CopySubresourceRegion(dst, 0, dst_x, dst_y, 0, src->texture(), 0, &sbox);
        }
    }
};

d3d_device::d3d_device()
    : impl_(new impl())
{
}

d3d_device::~d3d_device() {}

std::wstring d3d_device::adapter_name() const { return impl_->adaptor_name_; }

ID3D11Device* d3d_device::device() const { return impl_->device_.get(); }

std::shared_ptr<d3d_device_context> d3d_device::immedidate_context() { return impl_->ctx_; }

std::shared_ptr<d3d_texture2d> d3d_device::open_shared_texture(void* handle)
{
    return impl_->open_shared_texture(handle);
}

std::shared_ptr<d3d_texture2d> d3d_device::create_texture(int width, int height, DXGI_FORMAT format)
{
    return impl_->create_texture(width, height, format);
}

void d3d_device::copy_texture(ID3D11Texture2D*                      dst,
                              uint32_t                              dst_x,
                              uint32_t                              dst_y,
                              const std::shared_ptr<d3d_texture2d>& src,
                              uint32_t                              src_x,
                              uint32_t                              src_y,
                              uint32_t                              src_w,
                              uint32_t                              src_h)
{
    impl_->copy_texture(dst, dst_x, dst_y, src, src_x, src_y, src_w, src_h);
}

const std::shared_ptr<d3d_device>& d3d_device::get_device()
{
    static std::shared_ptr<d3d_device> device = []() -> std::shared_ptr<d3d_device> {
        try {
            return std::make_shared<d3d_device>();
        } catch (...) {
            CASPAR_LOG_CURRENT_EXCEPTION();
        }

        return nullptr;
    }();

    return device;
}
}}} // namespace caspar::accelerator::d3d
