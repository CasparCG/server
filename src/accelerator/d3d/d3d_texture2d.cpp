#include "d3d_texture2d.h"

#include <common/gl/gl_check.h>

#include <GL/glew.h>
#include <GL/wglew.h>
#include <atlcomcli.h>

#include "../ogl/util/device.h"

namespace caspar { namespace accelerator { namespace d3d {

d3d_texture2d::d3d_texture2d(ID3D11Texture2D* tex)
    : texture_(tex)
{
    share_handle_ = nullptr;

    D3D11_TEXTURE2D_DESC desc;
    texture_->GetDesc(&desc);
    width_  = desc.Width;
    height_ = desc.Height;
    format_ = desc.Format;

    if ((desc.MiscFlags & D3D11_RESOURCE_MISC_SHARED_NTHANDLE) == 0) {
        CASPAR_THROW_EXCEPTION(gl::ogl_exception() << msg_info("D3D texture is not sharable."));
    }

    {
        CComQIPtr<IDXGIResource1> res = texture_;
        if (res) {
            res->CreateSharedHandle(nullptr, DXGI_SHARED_RESOURCE_READ, nullptr, &share_handle_);
        }
    }

    if (share_handle_ == nullptr || !wglDXSetResourceShareHandleNV(texture_, share_handle_)) {
        CASPAR_THROW_EXCEPTION(gl::ogl_exception() << msg_info("Failed to setup shared d3d texture."));
    }
}

d3d_texture2d::~d3d_texture2d()
{
    const std::shared_ptr<ogl::device> ogl = ogl_.lock();
    if (ogl != nullptr) {
        // The cleanup must happen be done on the opengl thread
        ogl->dispatch_sync([&] {
            const std::shared_ptr<void> interop = ogl->d3d_interop();
            if (texture_handle_ != nullptr && interop != nullptr) {
                if (is_locked_) {
                    wglDXUnlockObjectsNV(interop.get(), 1, &texture_handle_);
                    is_locked_ = false;
                }

                wglDXUnregisterObjectNV(interop.get(), texture_handle_);
                texture_handle_ = nullptr;
            }

            if (gl_texture_id_ != 0) {
                GL(glDeleteTextures(1, &gl_texture_id_));
                gl_texture_id_ = 0;
            }

            // TODO: This appears to be leaking something opengl, but it is not clear what that is.

            if (share_handle_ != nullptr) {
                CloseHandle(share_handle_);
                share_handle_ = nullptr;
            }
        });
    }

    if (texture_ != nullptr) {
        texture_->Release();
        texture_ = nullptr;
    }
}

void d3d_texture2d::gen_gl_texture(std::shared_ptr<ogl::device> ogl)
{
    if (gl_texture_id_ != 0 || texture_ == nullptr)
        return;

    ogl_ = ogl;

    const std::shared_ptr<void> interop = ogl->d3d_interop();
    if (!interop) {
        CASPAR_THROW_EXCEPTION(gl::ogl_exception() << msg_info("d3d interop not setup to bind shared d3d texture."));
    }

    ogl->dispatch_sync([&] {
        GL(glGenTextures(1, &gl_texture_id_));

        texture_handle_ =
            wglDXRegisterObjectNV(interop.get(), texture_, gl_texture_id_, GL_TEXTURE_2D, WGL_ACCESS_READ_ONLY_NV);
        if (!texture_handle_) {
            GL(glDeleteTextures(1, &gl_texture_id_));
            gl_texture_id_ = 0;
            CASPAR_THROW_EXCEPTION(gl::ogl_exception() << msg_info("Failed to bind shared d3d texture."));
        }

        if (!wglDXLockObjectsNV(interop.get(), 1, &texture_handle_)) {
            wglDXUnregisterObjectNV(interop.get(), texture_handle_);
            texture_handle_ = nullptr;
            GL(glDeleteTextures(1, &gl_texture_id_));
            gl_texture_id_ = 0;
            CASPAR_THROW_EXCEPTION(gl::ogl_exception() << msg_info("Failed to lock shared d3d texture."));
        }

        is_locked_ = true;
    });
}

void d3d_texture2d::lock_gl()
{
    if (is_locked_)
        return;

    if (!texture_handle_ || gl_texture_id_ == 0) {
        CASPAR_THROW_EXCEPTION(gl::ogl_exception() << msg_info("texture is not ready to be locked."));
    }

    const std::shared_ptr<ogl::device> ogl = ogl_.lock();
    if (ogl == nullptr) {
        CASPAR_THROW_EXCEPTION(gl::ogl_exception() << msg_info("failed to lock opengl device."));
    }

    const std::shared_ptr<void> interop = ogl->d3d_interop();
    if (!interop) {
        CASPAR_THROW_EXCEPTION(gl::ogl_exception() << msg_info("d3d interop not setup to lock shared d3d texture."));
    }

    bool res = ogl->dispatch_sync([&] { return wglDXLockObjectsNV(interop.get(), 1, &texture_handle_); });
    if (!res) {
        CASPAR_THROW_EXCEPTION(gl::ogl_exception() << msg_info("Failed to lock shared d3d texture."));
    }

    is_locked_ = true;
}

void d3d_texture2d::unlock_gl()
{
    if (!is_locked_)
        return;

    if (!texture_handle_ || gl_texture_id_ == 0) {
        CASPAR_THROW_EXCEPTION(gl::ogl_exception() << msg_info("texture is not ready to be locked."));
    }

    const std::shared_ptr<ogl::device> ogl = ogl_.lock();
    if (ogl == nullptr) {
        CASPAR_THROW_EXCEPTION(gl::ogl_exception() << msg_info("failed to lock opengl device."));
    }

    const std::shared_ptr<void> interop = ogl->d3d_interop();
    if (!interop) {
        CASPAR_THROW_EXCEPTION(gl::ogl_exception() << msg_info("d3d interop not setup to unlock shared d3d texture."));
    }

    bool res = ogl->dispatch_sync([&] { return wglDXUnlockObjectsNV(interop.get(), 1, &texture_handle_); });
    if (!res) {
        CASPAR_THROW_EXCEPTION(gl::ogl_exception() << msg_info("Failed to unlock shared d3d texture."));
    }

    is_locked_ = false;
}

}}} // namespace caspar::accelerator::d3d
