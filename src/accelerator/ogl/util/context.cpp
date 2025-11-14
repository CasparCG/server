
#include "context.h"

#include <common/log.h>

#include <SFML/Window/Context.hpp>

#ifndef _MSC_VER
#include <EGL/egl.h>
#include <common/gl/gl_check.h>
#include <stdlib.h>
#endif
namespace caspar::accelerator::ogl {

struct device_context::impl
{
    virtual ~impl() {}
    virtual void bind()   = 0;
    virtual void unbind() = 0;
};
struct impl_sfml : public device_context::impl
{
    sf::Context device_;

    impl_sfml()
        : device_(sf::ContextSettings(0, 0, 0, 4, 5, sf::ContextSettings::Attribute::Core), 1, 1)
    {
        CASPAR_LOG(info) << L"Initializing OpenGL Device (sfml).";
    }

    virtual ~impl_sfml() {}

    virtual void bind() override { device_.setActive(true); }
    virtual void unbind() override { device_.setActive(false); }
};

#ifndef _MSC_VER
struct impl_egl : public device_context::impl
{
    EGLDisplay eglDisplay_;
    EGLContext eglContext_;

    impl_egl()
        : eglDisplay_(EGL_NO_DISPLAY)
        , eglContext_(EGL_NO_CONTEXT)
    {
        CASPAR_LOG(info) << L"Initializing OpenGL Device (EGL).";

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

    virtual ~impl_egl()
    {
        eglMakeCurrent(eglDisplay_, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);

        if (eglContext_ != EGL_NO_CONTEXT) {
            eglDestroyContext(eglDisplay_, eglContext_);
        }

        eglTerminate(eglDisplay_);
    }

    virtual void bind() override { eglMakeCurrent(eglDisplay_, EGL_NO_SURFACE, EGL_NO_SURFACE, eglContext_); }
    virtual void unbind() override { eglMakeCurrent(eglDisplay_, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT); }
};
#endif

#ifndef _MSC_VER
device_context::device_context()
    : impl_(std::getenv("DISPLAY") == nullptr ? spl::make_shared<device_context::impl, impl_egl>()
                                              : spl::make_shared<device_context::impl, impl_sfml>())
{
}
#else
device_context::device_context()
    : impl_(new impl_sfml())
{
}
#endif

device_context::~device_context() {}

void device_context::bind() { impl_->bind(); }
void device_context::unbind() { impl_->unbind(); }

} // namespace caspar::accelerator::ogl