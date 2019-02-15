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
#include "shader.h"

#include <common/gl/gl_check.h>

#include <GL/glew.h>

#include <unordered_map>

namespace caspar { namespace accelerator { namespace ogl {

struct shader::impl
{
    GLuint                                 program_;
    std::unordered_map<std::string, GLint> uniform_locations_;
    std::unordered_map<std::string, GLint> attrib_locations_;

    impl(const impl&) = delete;
    impl& operator=(const impl&) = delete;

  public:
    impl(const std::string& vertex_source_str, const std::string& fragment_source_str)
        : program_(0)
    {
        GLint success;

        const char* vertex_source = vertex_source_str.c_str();

        auto vertex_shader = glCreateShaderObjectARB(GL_VERTEX_SHADER_ARB);

        GL(glShaderSourceARB(vertex_shader, 1, &vertex_source, NULL));
        GL(glCompileShaderARB(vertex_shader));

        GL(glGetObjectParameterivARB(vertex_shader, GL_OBJECT_COMPILE_STATUS_ARB, &success));
        if (success == GL_FALSE) {
            char info[2048];
            GL(glGetInfoLogARB(vertex_shader, sizeof(info), 0, info));
            GL(glDeleteObjectARB(vertex_shader));
            std::stringstream str;
            str << "Failed to compile vertex shader:" << std::endl << info << std::endl;
            CASPAR_THROW_EXCEPTION(caspar_exception() << msg_info(str.str()));
        }

        const char* fragment_source = fragment_source_str.c_str();

        auto fragmemt_shader = glCreateShaderObjectARB(GL_FRAGMENT_SHADER_ARB);

        GL(glShaderSourceARB(fragmemt_shader, 1, &fragment_source, NULL));
        GL(glCompileShaderARB(fragmemt_shader));

        GL(glGetObjectParameterivARB(fragmemt_shader, GL_OBJECT_COMPILE_STATUS_ARB, &success));
        if (success == GL_FALSE) {
            char info[2048];
            GL(glGetInfoLogARB(fragmemt_shader, sizeof(info), 0, info));
            GL(glDeleteObjectARB(fragmemt_shader));
            std::stringstream str;
            str << "Failed to compile fragment shader:" << std::endl << info << std::endl;
            CASPAR_THROW_EXCEPTION(caspar_exception() << msg_info(str.str()));
        }

        program_ = glCreateProgramObjectARB();

        GL(glAttachObjectARB(program_, vertex_shader));
        GL(glAttachObjectARB(program_, fragmemt_shader));

        GL(glLinkProgramARB(program_));

        GL(glDeleteObjectARB(vertex_shader));
        GL(glDeleteObjectARB(fragmemt_shader));

        GL(glGetObjectParameterivARB(program_, GL_OBJECT_LINK_STATUS_ARB, &success));
        if (success == GL_FALSE) {
            char info[2048];
            GL(glGetInfoLogARB(program_, sizeof(info), 0, info));
            GL(glDeleteObjectARB(program_));
            std::stringstream str;
            str << "Failed to link shader program:" << std::endl << info << std::endl;
            CASPAR_THROW_EXCEPTION(caspar_exception() << msg_info(str.str()));
        }
        GL(glUseProgramObjectARB(program_));
    }

    ~impl() { glDeleteProgram(program_); }

    GLint get_uniform_location(const char* name)
    {
        auto it = uniform_locations_.find(name);
        if (it == uniform_locations_.end())
            it = uniform_locations_.insert(std::make_pair(name, glGetUniformLocation(program_, name))).first;
        return it->second;
    }

    GLint get_attrib_location(const char* name)
    {
        auto it = attrib_locations_.find(name);
        if (it == attrib_locations_.end())
            it = attrib_locations_.insert(std::make_pair(name, glGetAttribLocation(program_, name))).first;
        return it->second;
    }

    void set(const std::string& name, bool value) { set(name, value ? 1 : 0); }

    void set(const std::string& name, int value) { GL(glUniform1i(get_uniform_location(name.c_str()), value)); }

    void set(const std::string& name, float value) { GL(glUniform1f(get_uniform_location(name.c_str()), value)); }

    void set(const std::string& name, double value0, double value1)
    {
        GL(glUniform2f(get_uniform_location(name.c_str()), static_cast<float>(value0), static_cast<float>(value1)));
    }

    void set(const std::string& name, double value)
    {
        GL(glUniform1f(get_uniform_location(name.c_str()), static_cast<float>(value)));
    }

    void use() { GL(glUseProgramObjectARB(program_)); }
};

shader::shader(const std::string& vertex_source_str, const std::string& fragment_source_str)
    : impl_(new impl(vertex_source_str, fragment_source_str))
{
}
shader::~shader() {}
void  shader::set(const std::string& name, bool value) { impl_->set(name, value); }
void  shader::set(const std::string& name, int value) { impl_->set(name, value); }
void  shader::set(const std::string& name, float value) { impl_->set(name, value); }
void  shader::set(const std::string& name, double value0, double value1) { impl_->set(name, value0, value1); }
void  shader::set(const std::string& name, double value) { impl_->set(name, value); }
GLint shader::get_attrib_location(const char* name) { return impl_->get_attrib_location(name); }
int   shader::id() const { return impl_->program_; }
void  shader::use() const { impl_->use(); }

}}} // namespace caspar::accelerator::ogl
