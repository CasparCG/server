#include "../StdAfx.h"

#include "gpu_frame_transform.h"

#include "../../common/exception/exceptions.h"
#include "../../common/gl/gl_check.h"

#include <Glee.h>

#include <fstream>
#include <unordered_map>

namespace caspar { namespace core {
	
class shader_program
{
public:
	shader_program(const std::string& fragment_source_str)
	{
		try
		{		
			const char* fragment_source = fragment_source_str.c_str();
			static const char* vertex_source = 
				"void main()"
				"{"
					"gl_TexCoord[0] = gl_MultiTexCoord0;"
					"gl_FrontColor = gl_Color;"
					"gl_Position = ftransform();"
				"}";

			program_ = glCreateProgramObjectARB();

			auto vertex_shader   = glCreateShaderObjectARB(GL_VERTEX_SHADER_ARB);
			auto fragmemt_shader = glCreateShaderObjectARB(GL_FRAGMENT_SHADER_ARB);

			GL(glShaderSourceARB(vertex_shader, 1, &vertex_source,   NULL));
			GL(glShaderSourceARB(fragmemt_shader, 1, &fragment_source, NULL));
			GL(glCompileShaderARB(vertex_shader));
			GL(glCompileShaderARB(fragmemt_shader));

			GLint success;
			GL(glGetObjectParameterivARB(vertex_shader, GL_OBJECT_COMPILE_STATUS_ARB, &success));
			if (success == GL_FALSE)
			{
				char log[1024];
				GL(glGetInfoLogARB(vertex_shader, sizeof(log), 0, log));
				GL(glDeleteObjectARB(vertex_shader));
				GL(glDeleteObjectARB(fragmemt_shader));
				GL(glDeleteObjectARB(program_));
				std::stringstream str;
				str << "Failed to compile vertex shader:" << std::endl << log << std::endl;
				BOOST_THROW_EXCEPTION(caspar_exception() << msg_info(str.str()));
			}
			GL(glGetObjectParameterivARB(fragmemt_shader, GL_OBJECT_COMPILE_STATUS_ARB, &success));
			if (success == GL_FALSE)
			{
				char log[1024];
				GL(glGetInfoLogARB(fragmemt_shader, sizeof(log), 0, log));
				GL(glDeleteObjectARB(vertex_shader));
				GL(glDeleteObjectARB(fragmemt_shader));
				GL(glDeleteObjectARB(program_));
				std::stringstream str;
				str << "Failed to compile fragment shader:" << std::endl << log << std::endl;
				BOOST_THROW_EXCEPTION(caspar_exception() << msg_info(str.str()));
			}

			GL(glAttachObjectARB(program_, vertex_shader));
			GL(glAttachObjectARB(program_, fragmemt_shader));

			GL(glDeleteObjectARB(vertex_shader));
			GL(glDeleteObjectARB(fragmemt_shader));

			GL(glLinkProgramARB(program_));

			GL(glGetObjectParameterivARB(program_, GL_OBJECT_LINK_STATUS_ARB, &success));
			if (success == GL_FALSE)
			{
				char log[1024];
				GL(glGetInfoLogARB(program_, sizeof(log), 0, log));
				CASPAR_LOG(warning) << "Failed to link shader:" << std::endl
						<< log << std::endl;
				GL(glDeleteObjectARB(program_));
				BOOST_THROW_EXCEPTION(caspar_exception());
			}
			GL(glUseProgramObjectARB(program_));
			glUniform1i(glGetUniformLocation(program_, "tex0"), 0);
			glUniform1i(glGetUniformLocation(program_, "tex1"), 1);
			glUniform1i(glGetUniformLocation(program_, "tex2"), 2);
			glUniform1i(glGetUniformLocation(program_, "tex3"), 3);
		}
		catch(...)
		{
			CASPAR_LOG_CURRENT_EXCEPTION();
			throw;
		}
	}

	void use()
	{
		GL(glUseProgramObjectARB(program_));
	}

private:
	GLuint program_;
};
typedef std::shared_ptr<shader_program> shader_program_ptr;

struct gpu_frame_transform::implementation
{
	implementation() : current_(pixel_format::invalid_pixel_format)
	{
		std::string common = 
			"uniform sampler2D tex0;"
			"uniform sampler2D tex1;"
			"uniform sampler2D tex2;"
			"uniform sampler2D tex3;"

			"vec4 yuva_to_bgra(float y, float u, float v, float a)"
			"{"
			   "vec4 color;"
   
			   "u -= 0.5;"
			   "v -= 0.5;"

			   "color.r = clamp(y + 1.370705 * v, 0.0 , 1.0);"
			   "color.g = clamp(y - 0.698001 * v - 0.337633 * u, 0.0 , 1.0);"
			   "color.b = clamp(y + 1.732446 * u, 0.0 , 1.0);"
			   "color.a = a;"

			  " return color;"
			"}";

		shaders_[pixel_format::abgr] = std::make_shared<shader_program>
		(
			common +

			"void main()"
			"{"
				"gl_FragColor = texture2D(tex0, gl_TexCoord[0].st).argb * gl_Color;"
			"}"
		);
		shaders_[pixel_format::argb] = std::make_shared<shader_program>
		(
			common +

			"void main()"
			"{"
				"gl_FragColor = texture2D(tex0, gl_TexCoord[0].st).grab * gl_Color;"
			"}"
		);
		shaders_[pixel_format::bgra] = std::make_shared<shader_program>
		(
			common +

			"void main()"
			"{"
				"gl_FragColor = texture2D(tex0, gl_TexCoord[0].st).rgba * gl_Color;"
			"}"
		);
		shaders_[pixel_format::rgba] = std::make_shared<shader_program>
		(
			common +

			"void main()"
			"{"
				"gl_FragColor = texture2D(tex0, gl_TexCoord[0].st).bgra * gl_Color;"
			"}"
		);
		shaders_[pixel_format::yuv] = std::make_shared<shader_program>
		(
			common +

			"void main()"
			"{"
				"float y = texture2D(tex0, gl_TexCoord[0].st).r;"
				"float u = texture2D(tex1, gl_TexCoord[0].st).r;"
				"float v = texture2D(tex2, gl_TexCoord[0].st).r;"
				"gl_FragColor = yuva_to_bgra(y, u , v, 1.0) * gl_Color;"
			"}"
		);
		shaders_[pixel_format::yuva] = std::make_shared<shader_program>
		(
			common +

			"void main()"
			"{"
				"float y = texture2D(tex0, gl_TexCoord[0].st).r;"
				"float u = texture2D(tex1, gl_TexCoord[0].st).r;"
				"float v = texture2D(tex2, gl_TexCoord[0].st).r;"
				"float a = texture2D(tex3, gl_TexCoord[0].st).r;"
				"gl_FragColor = yuva_to_bgra(y, u, v, a) * gl_Color;"
			"}"
		);
	}

	void set_pixel_format(pixel_format format)
	{
		if(current_ == format)
			return;
		current_ = format;
		shaders_[format]->use();
	}

	pixel_format current_;
	std::map<pixel_format, shader_program_ptr> shaders_;
};

gpu_frame_transform::gpu_frame_transform() : impl_(new implementation()){}
void gpu_frame_transform::set_pixel_format(pixel_format format){impl_->set_pixel_format(format);}

}}