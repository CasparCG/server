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
#include "compute_shader.h"

#include <common/gl/gl_check.h>

#include <GL/glew.h>

#include <unordered_map>

namespace caspar { namespace accelerator { namespace ogl {

struct compute_shader::impl
{
    GLuint                                 program_;
    std::unordered_map<std::string, GLint> uniform_locations_;
    std::unordered_map<std::string, GLint> attrib_locations_;

    impl(const impl&)            = delete;
    impl& operator=(const impl&) = delete;

  public:
    impl(const std::string& compute_source_str)
        : program_(0)
    {
        int work_grp_cnt[3];

        glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 0, &work_grp_cnt[0]);
        glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 1, &work_grp_cnt[1]);
        glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 2, &work_grp_cnt[2]);

        printf(
            "max global (total) work group counts x:%i y:%i z:%i\n", work_grp_cnt[0], work_grp_cnt[1], work_grp_cnt[2]);

        GLint success;

        const char* compute_source = compute_source_str.c_str();

        auto compute_shader = glCreateShader(GL_COMPUTE_SHADER);

        GL(glShaderSource(compute_shader, 1, &compute_source, NULL));
        GL(glCompileShader(compute_shader));

        GL(glGetObjectParameterivARB(compute_shader, GL_OBJECT_COMPILE_STATUS_ARB, &success));
        if (success == GL_FALSE) {
            char info[2048];
            GL(glGetInfoLogARB(compute_shader, sizeof(info), 0, info));
            GL(glDeleteObjectARB(compute_shader));
            std::stringstream str;
            str << "Failed to compile compute shader:" << std::endl << info << std::endl;
            CASPAR_THROW_EXCEPTION(caspar_exception() << msg_info(str.str()));
        }

        program_ = glCreateProgram();

        GL(glAttachShader(program_, compute_shader));
        GL(glLinkProgram(program_));

        GL(glDeleteShader(compute_shader));

        GL(glGetObjectParameterivARB(program_, GL_OBJECT_LINK_STATUS_ARB, &success));
        if (success == GL_FALSE) {
            char info[2048];
            GL(glGetInfoLogARB(program_, sizeof(info), 0, info));
            GL(glDeleteObjectARB(program_));
            std::stringstream str;
            str << "Failed to link shader program:" << std::endl << info << std::endl;
            CASPAR_THROW_EXCEPTION(caspar_exception() << msg_info(str.str()));
        }
        GL(glUseProgram(program_));
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

    void use() { GL(glUseProgram(program_)); }
};

compute_shader::compute_shader(const std::string& compute_source_str)
    : impl_(new impl(compute_source_str))
{
}
compute_shader::~compute_shader() {}
void  compute_shader::set(const std::string& name, bool value) { impl_->set(name, value); }
void  compute_shader::set(const std::string& name, int value) { impl_->set(name, value); }
void  compute_shader::set(const std::string& name, float value) { impl_->set(name, value); }
void  compute_shader::set(const std::string& name, double value0, double value1) { impl_->set(name, value0, value1); }
void  compute_shader::set(const std::string& name, double value) { impl_->set(name, value); }
GLint compute_shader::get_attrib_location(const char* name) { return impl_->get_attrib_location(name); }
int   compute_shader::id() const { return impl_->program_; }
void  compute_shader::use() const { impl_->use(); }

}}} // namespace caspar::accelerator::ogl
