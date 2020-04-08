#include "d3d_texture2d.h"

#include <common/gl/gl_check.h>

#include <GL/glew.h>
#include <GL/wglew.h>

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

d3d_texture2d::~d3d_texture2d()
{
    if (gl_texture_id_ != 0)
        glDeleteTextures(1, &gl_texture_id_);
}

void d3d_texture2d::gen_gl_texture(const std::shared_ptr<void>& interop)
{
    if (gl_texture_id_ == 0) {
        GL(glGenTextures(1, &gl_texture_id_));
        void* tex_handle = wglDXRegisterObjectNV(
            interop.get(), texture_.get(), gl_texture_id_, GL_TEXTURE_2D, WGL_ACCESS_READ_ONLY_NV);
        if (!tex_handle)
            CASPAR_THROW_EXCEPTION(gl::ogl_exception() << msg_info("Failed to bind shared d3d texture."));

        texture_handle_ = std::shared_ptr<void>(tex_handle, [=](void* p) {
            wglDXUnlockObjectsNV(interop.get(), 1, &p);
            wglDXUnregisterObjectNV(interop.get(), p);
        });

        if (!wglDXLockObjectsNV(interop.get(), 1, &tex_handle))
            CASPAR_THROW_EXCEPTION(gl::ogl_exception() << msg_info("Failed to lock shared d3d texture."));
    }
}

}}} // namespace caspar::accelerator::d3d
