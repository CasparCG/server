
#include "context.h"

#include <common/gl/gl_check.h>
#include <common/log.h>

#include <EGL/egl.h>

namespace caspar::accelerator::ogl {

struct device_context::impl
{
    EGLDisplay eglDisplay_;
    EGLContext eglContext_;

    impl()
        : eglDisplay_(EGL_NO_DISPLAY)
        , eglContext_(EGL_NO_CONTEXT)
    {
        CASPAR_LOG(info) << L"Initializing OpenGL Device.";

        eglDisplay_ = eglGetDisplay(EGL_DEFAULT_DISPLAY);

        EGLint major, minor;
        eglInitialize(eglDisplay_, &major, &minor);

        const EGLint configAttribs[] = {EGL_SURFACE_TYPE,
                                        EGL_PBUFFER_BIT,
                                        EGL_BLUE_SIZE,
                                        8,
                                        EGL_GREEN_SIZE,
                                        8,
                                        EGL_RED_SIZE,
                                        8,
                                        EGL_RENDERABLE_TYPE,
                                        EGL_OPENGL_BIT,
                                        EGL_NONE};

        EGLint    numConfigs;
        EGLConfig eglConfig;
        if (!eglChooseConfig(eglDisplay_, configAttribs, &eglConfig, 1, &numConfigs)) {
            CASPAR_THROW_EXCEPTION(gl::ogl_exception() << msg_info("Failed to initialize OpenGL: eglChooseConfig"));
        }

        if (!eglBindAPI(EGL_OPENGL_API)) {
            CASPAR_THROW_EXCEPTION(gl::ogl_exception() << msg_info("Failed to initialize OpenGL: eglBindAPI"));
        }

        eglContext_ = eglCreateContext(eglDisplay_, eglConfig, EGL_NO_CONTEXT, NULL);
        if (eglContext_ == EGL_NO_CONTEXT) {
            CASPAR_THROW_EXCEPTION(gl::ogl_exception() << msg_info("Failed to initialize OpenGL: eglCreateContext"));
        }

        if (!eglMakeCurrent(eglDisplay_, EGL_NO_SURFACE, EGL_NO_SURFACE, eglContext_)) {
            CASPAR_THROW_EXCEPTION(gl::ogl_exception() << msg_info("Failed to initialize OpenGL: eglMakeCurrent"));
        }
    }

    ~impl()
    {
        eglMakeCurrent(eglDisplay_, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);

        if (eglContext_ != EGL_NO_CONTEXT) {
            eglDestroyContext(eglDisplay_, eglContext_);
        }

        eglTerminate(eglDisplay_);
    }
};

device_context::device_context()
    : impl_(new impl())
{
}
device_context::~device_context() {}

void device_context::bind() { eglMakeCurrent(impl_->eglDisplay_, EGL_NO_SURFACE, EGL_NO_SURFACE, impl_->eglContext_); }
void device_context::unbind() { eglMakeCurrent(impl_->eglDisplay_, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT); }

} // namespace caspar::accelerator::ogl