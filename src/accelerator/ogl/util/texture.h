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

#pragma once

#include <memory>

namespace caspar { namespace accelerator { namespace ogl {

class texture final
{
  public:
    texture(int width, int height, int stride);
    texture(const texture&) = delete;
    texture(texture&& other);
    ~texture();

    texture& operator=(const texture&) = delete;
    texture& operator                  =(texture&& other);

#ifdef WIN32
    void copy_from(int source);
#endif
    void copy_from(class buffer& source);
    void copy_to(class buffer& dest);

    void attach();
    void clear();
    void bind(int index);
    void unbind();

    int width() const;
    int height() const;
    int stride() const;
    int size() const;
    int id() const;

  private:
    struct impl;
    std::unique_ptr<impl> impl_;
};

}}} // namespace caspar::accelerator::ogl
