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
#include "buffer.h"

#include <common/gl/gl_check.h>

#include <tbb/atomic.h>

#include <GL/glew.h>

namespace caspar { namespace accelerator { namespace ogl {

static tbb::atomic<int>         g_w_total_count;
static tbb::atomic<std::size_t> g_w_total_size;
static tbb::atomic<int>         g_r_total_count;
static tbb::atomic<std::size_t> g_r_total_size;

struct buffer::impl
{
    GLuint     id_     = 0;
    GLsizei    size_   = 0;
    void*      data_   = nullptr;
    bool       write_  = false;
    GLenum     target_ = 0;
    GLbitfield flags_  = 0;

    impl(const impl&) = delete;
    impl& operator=(const impl&) = delete;

  public:
    impl(int size, bool write)
        : size_(size)
        , write_(write)
        , target_(!write ? GL_PIXEL_PACK_BUFFER : GL_PIXEL_UNPACK_BUFFER)
        , flags_(GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT | (write ? GL_MAP_WRITE_BIT : GL_MAP_READ_BIT))
    {
        GL(glCreateBuffers(1, &id_));
        GL(glNamedBufferStorage(id_, size_, nullptr, flags_));
        data_ = GL2(glMapNamedBufferRange(id_, 0, size_, flags_));

        (write ? g_w_total_count : g_r_total_count)++;
        (write ? g_w_total_size : g_r_total_size) += size_;
    }

    ~impl()
    {
        GL(glUnmapNamedBuffer(id_));
        glDeleteBuffers(1, &id_);

        (write_ ? g_w_total_size : g_r_total_size) -= size_;
        (write_ ? g_w_total_count : g_r_total_count)--;
    }

    void bind() { GL(glBindBuffer(target_, id_)); }

    void unbind() { GL(glBindBuffer(target_, 0)); }
};

buffer::buffer(int size, bool write)
    : impl_(new impl(size, write))
{
}
buffer::buffer(buffer&& other)
    : impl_(std::move(other.impl_))
{
}
buffer::~buffer() {}
buffer& buffer::operator=(buffer&& other)
{
    impl_ = std::move(other.impl_);
    return *this;
}
void* buffer::data() { return impl_->data_; }
bool  buffer::write() const { return impl_->write_; }
int   buffer::size() const { return impl_->size_; }
void  buffer::bind() { return impl_->bind(); }
void  buffer::unbind() { return impl_->unbind(); }
int   buffer::id() const { return impl_->id_; }

boost::property_tree::wptree buffer::info()
{
    boost::property_tree::wptree info;

    info.add(L"total_read_count", g_r_total_count);
    info.add(L"total_write_count", g_w_total_count);
    info.add(L"total_read_size", g_r_total_size);
    info.add(L"total_write_size", g_w_total_size);

    return info;
}

}}} // namespace caspar::accelerator::ogl
