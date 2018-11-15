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

#include <GL/glew.h>
#include <memory>
#include <string>
#include <type_traits>

namespace caspar { namespace accelerator { namespace ogl {

class shader final
{
    shader(const shader&);
    shader& operator=(const shader&);

  public:
    shader(const std::string& vertex_source_str, const std::string& fragment_source_str);
    ~shader();

    void set(const std::string& name, bool value);
    void set(const std::string& name, int value);
    void set(const std::string& name, float value);
    void set(const std::string& name, double value0, double value1);
    void set(const std::string& name, double value);

    GLint get_attrib_location(const char* name);

    template <typename E>
    typename std::enable_if<std::is_enum<E>::value, void>::type set(const std::string& name, E value)
    {
        set(name, static_cast<typename std::underlying_type<E>::type>(value));
    }

    void use() const;

    int id() const;

  private:
    struct impl;
    std::unique_ptr<impl> impl_;
};

}}} // namespace caspar::accelerator::ogl
