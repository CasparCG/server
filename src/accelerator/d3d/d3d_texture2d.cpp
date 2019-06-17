#include "d3d_texture2d.h"

namespace caspar { namespace accelerator { namespace d3d {
d3d_texture2d::d3d_texture2d(ID3D11Texture2D* tex)
    : texture_(std::shared_ptr<ID3D11Texture2D>(tex, [](ID3D11Texture2D* p) {
        if (p)
            p->Release();
    }))
{
    share_handle_ = nullptr;

    IDXGIResource* res = nullptr;
    if (SUCCEEDED(texture_->QueryInterface(__uuidof(IDXGIResource), reinterpret_cast<void**>(&res)))) {
        res->GetSharedHandle(&share_handle_);
        res->Release();
    }

    D3D11_TEXTURE2D_DESC desc;
    texture_->GetDesc(&desc);
    width_  = desc.Width;
    height_ = desc.Height;
    format_ = desc.Format;
}

}}} // namespace caspar::accelerator::d3d
