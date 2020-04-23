/*
 * Copyright (c) 2011 Sveriges Television AB <info@casparcg.com>
 *
 * This file is part of CasparCG (www.casparcg.com).
 *
 * CasparCG is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * CasparCG is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with CasparCG. If not, see <http://www.gnu.org/licenses/>.
 *
 * Author: Robert Nagy, ronag89@gmail.com
 */
#include "texture.h"

#include "buffer.h"

#include <common/gl/gl_check.h>

#include <GL/glew.h>

namespace caspar { namespace accelerator { namespace ogl {

static GLenum FORMAT[]          = {0, GL_RED, GL_RG, GL_BGR, GL_BGRA};
static GLenum INTERNAL_FORMAT[] = {0, GL_R8, GL_RG8, GL_RGB8, GL_RGBA8};
static GLenum TYPE[] = {0, GL_UNSIGNED_BYTE, GL_UNSIGNED_BYTE, GL_UNSIGNED_BYTE, GL_UNSIGNED_INT_8_8_8_8_REV};

struct texture::impl
{
    GLuint  id_     = 0;
    GLsizei width_  = 0;
    GLsizei height_ = 0;
    GLsizei stride_ = 0;
    GLsizei size_   = 0;

    impl(const impl&) = delete;
    impl& operator=(const impl&) = delete;

  public:
    impl(int width, int height, int stride)
        : width_(width)
        , height_(height)
        , stride_(stride)
        , size_(width * height * stride)
    {
        GL(glCreateTextures(GL_TEXTURE_2D, 1, &id_));
        GL(glTextureParameteri(id_, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
        GL(glTextureParameteri(id_, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
        GL(glTextureParameteri(id_, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
        GL(glTextureParameteri(id_, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
        GL(glTextureStorage2D(id_, 1, INTERNAL_FORMAT[stride_], width_, height_));
    }

    ~impl() { glDeleteTextures(1, &id_); }

    void bind() { GL(glBindTexture(GL_TEXTURE_2D, id_)); }

    void bind(int index)
    {
        GL(glActiveTexture(GL_TEXTURE0 + index));
        bind();
    }

    void unbind() { GL(glBindTexture(GL_TEXTURE_2D, 0)); }

    void attach() { GL(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + 0, GL_TEXTURE_2D, id_, 0)); }

    void clear() { GL(glClearTexImage(id_, 0, FORMAT[stride_], TYPE[stride_], nullptr)); }

#ifdef WIN32
    void copy_from(int texture_id)
    {
        GL(glCopyImageSubData(
            texture_id, GL_TEXTURE_2D, 0, 0, 0, 0, id_, GL_TEXTURE_2D, 0, 0, 0, 0, width_, height_, 1));
    }
#endif

    void copy_from(buffer& src)
    {
        src.bind();

        if (width_ % 16 > 0) {
            glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        } else {
            glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
        }

        GL(glTextureSubImage2D(id_, 0, 0, 0, width_, height_, FORMAT[stride_], TYPE[stride_], nullptr));

        src.unbind();
    }

    void copy_to(buffer& dst)
    {
        dst.bind();
        GL(glGetTextureImage(id_, 0, FORMAT[stride_], TYPE[stride_], size_, nullptr));
        dst.unbind();
    }
};

texture::texture(int width, int height, int stride)
    : impl_(new impl(width, height, stride))
{
}
texture::texture(texture&& other)
    : impl_(std::move(other.impl_))
{
}
texture::~texture() {}
texture& texture::operator=(texture&& other)
{
    impl_ = std::move(other.impl_);
    return *this;
}
void texture::bind(int index) { impl_->bind(index); }
void texture::unbind() { impl_->unbind(); }
void texture::attach() { impl_->attach(); }
void texture::clear() { impl_->clear(); }
#ifdef WIN32
void texture::copy_from(int source) { impl_->copy_from(source); }
#endif
void texture::copy_from(buffer& source) { impl_->copy_from(source); }
void texture::copy_to(buffer& dest) { impl_->copy_to(dest); }
int  texture::width() const { return impl_->width_; }
int  texture::height() const { return impl_->height_; }
int  texture::stride() const { return impl_->stride_; }
int  texture::size() const { return impl_->width_ * impl_->height_ * impl_->stride_; }
int  texture::id() const { return impl_->id_; }

}}} // namespace caspar::accelerator::ogl
