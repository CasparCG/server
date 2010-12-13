#include "../StdAfx.h"

#include "shader_program.h"

#include "../exception/exceptions.h"
#include "utility.h"

#include <Glee.h>

#include <boost/noncopyable.hpp>

namespace caspar { namespace gl {

shader_program& shader_program::operator=(shader_program&& other) 
{
	program_ = other.program_; 
	other.program_ = 0; 
	return *this;
}

shader_program::shader_program(const std::string& vertex_source_str, const std::string& fragment_source_str) : program_(0)
{
	GLint success;
	
	const char* vertex_source = vertex_source_str.c_str();
						
	auto vertex_shader = glCreateShaderObjectARB(GL_VERTEX_SHADER_ARB);
					
	GL(glShaderSourceARB(vertex_shader, 1, &vertex_source, NULL));
	GL(glCompileShaderARB(vertex_shader));

	GL(glGetObjectParameterivARB(vertex_shader, GL_OBJECT_COMPILE_STATUS_ARB, &success));
	if (success == GL_FALSE)
	{
		char info[2048];
		GL(glGetInfoLogARB(vertex_shader, sizeof(info), 0, info));
		GL(glDeleteObjectARB(vertex_shader));
		std::stringstream str;
		str << "Failed to compile vertex shader:" << std::endl << info << std::endl;
		BOOST_THROW_EXCEPTION(gl::gl_error() << msg_info(str.str()));
	}
			
	const char* fragment_source = fragment_source_str.c_str();
						
	auto fragmemt_shader = glCreateShaderObjectARB(GL_FRAGMENT_SHADER_ARB);
					
	GL(glShaderSourceARB(fragmemt_shader, 1, &fragment_source, NULL));
	GL(glCompileShaderARB(fragmemt_shader));

	GL(glGetObjectParameterivARB(fragmemt_shader, GL_OBJECT_COMPILE_STATUS_ARB, &success));
	if (success == GL_FALSE)
	{
		char info[2048];
		GL(glGetInfoLogARB(fragmemt_shader, sizeof(info), 0, info));
		GL(glDeleteObjectARB(fragmemt_shader));
		std::stringstream str;
		str << "Failed to compile fragment shader:" << std::endl << info << std::endl;
		BOOST_THROW_EXCEPTION(gl::gl_error() << msg_info(str.str()));
	}
			
	program_ = glCreateProgramObjectARB();
			
	GL(glAttachObjectARB(program_, vertex_shader));
	GL(glAttachObjectARB(program_, fragmemt_shader));

	GL(glLinkProgramARB(program_));
			
	GL(glDeleteObjectARB(vertex_shader));
	GL(glDeleteObjectARB(fragmemt_shader));

	GL(glGetObjectParameterivARB(program_, GL_OBJECT_LINK_STATUS_ARB, &success));
	if (success == GL_FALSE)
	{
		char info[2048];
		GL(glGetInfoLogARB(program_, sizeof(info), 0, info));
		GL(glDeleteObjectARB(program_));
		std::stringstream str;
		str << "Failed to link shader program:" << std::endl << info << std::endl;
		BOOST_THROW_EXCEPTION(gl::gl_error() << msg_info(str.str()));
	}
	GL(glUseProgramObjectARB(program_));
	glUniform1i(glGetUniformLocation(program_, "plane[0]"), 0);
	glUniform1i(glGetUniformLocation(program_, "plane[1]"), 1);
	glUniform1i(glGetUniformLocation(program_, "plane[2]"), 2);
	glUniform1i(glGetUniformLocation(program_, "plane[3]"), 3);
}

shader_program::~shader_program()
{
	glDeleteProgram(program_);
}

void shader_program::use()
{	
	GL(glUseProgramObjectARB(program_));		
}

}}