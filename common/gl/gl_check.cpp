////////////////////////////////////////////////////////////
//
// SFML - Simple and Fast Multimedia Library
// Copyright (C) 2007-2009 Laurent Gomila (laurent.gom@gmail.com)
//
// This software is provided 'as-is', without any express or implied warranty.
// In no event will the authors be held liable for any damages arising from the use of this software.
//
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it freely,
// subject to the following restrictions:
//
// 1. The origin of this software must not be misrepresented;
//    you must not claim that you wrote the original software.
//    If you use this software in a product, an acknowledgment
//    in the product documentation would be appreciated but is not required.
//
// 2. Altered source versions must be plainly marked as such,
//    and must not be misrepresented as being the original software.
//
// 3. This notice may not be removed or altered from any source distribution.
//
////////////////////////////////////////////////////////////

#pragma once

#include "../stdafx.h"

#include "gl_check.h"

#include <GL/glew.h>

#include "../exception/exceptions.h"
#include "../log/log.h"

#include <boost/lexical_cast.hpp>

namespace caspar { namespace gl {	

void SMFL_GLCheckError(const std::string& expr, const std::string& file, unsigned int line)
{
	// Get the last error
	GLenum LastErrorCode = GL_NO_ERROR;

	for(GLenum ErrorCode = glGetError(); ErrorCode != GL_NO_ERROR; ErrorCode = glGetError())
	{
		CASPAR_LOG(error) << "OpenGL Error: " << ErrorCode << L" " << glewGetErrorString(ErrorCode);
		LastErrorCode = ErrorCode;
	}

	if (LastErrorCode != GL_NO_ERROR)
	{
		// Decode the error code
		switch (LastErrorCode)
		{
			case GL_INVALID_ENUM :
				BOOST_THROW_EXCEPTION(ogl_invalid_enum()
					<< msg_info("an unacceptable value has been specified for an enumerated argument")
					<< errorstr("GL_INVALID_ENUM")
					<< line_info(line)
					<< source_info(file));

			case GL_INVALID_VALUE :
				BOOST_THROW_EXCEPTION(ogl_invalid_value()
					<< msg_info("a numeric argument is out of range")
					<< errorstr("GL_INVALID_VALUE")
					<< line_info(line)
					<< source_info(file));

			case GL_INVALID_OPERATION :
				BOOST_THROW_EXCEPTION(ogl_invalid_operation()
					<< msg_info("the specified operation is not allowed in the current state")
					<< errorstr("GL_INVALID_OPERATION")
					<< line_info(line)
					<< source_info(file));

			case GL_STACK_OVERFLOW :
				BOOST_THROW_EXCEPTION(ogl_stack_overflow()
					<< msg_info("this command would cause a stack overflow")
					<< errorstr("GL_STACK_OVERFLOW")
					<< line_info(line)
					<< source_info(file));

			case GL_STACK_UNDERFLOW :
				BOOST_THROW_EXCEPTION(ogl_stack_underflow()
					<< msg_info("this command would cause a stack underflow")
					<< errorstr("GL_STACK_UNDERFLOW")
					<< line_info(line)
					<< source_info(file));

			case GL_OUT_OF_MEMORY :
				BOOST_THROW_EXCEPTION(ogl_out_of_memory()
					<< msg_info("there is not enough memory left to execute the command")
					<< errorstr("GL_OUT_OF_MEMORY")
					<< line_info(line)
					<< source_info(file));

			case GL_INVALID_FRAMEBUFFER_OPERATION :
				BOOST_THROW_EXCEPTION(ogl_stack_underflow()
					<< msg_info("the object bound to FRAMEBUFFER_BINDING is not \"framebuffer complete\"")
					<< errorstr("GL_INVALID_FRAMEBUFFER_OPERATION")
					<< line_info(line)
					<< source_info(file));
		}
	}
}

}}