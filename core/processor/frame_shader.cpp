#include "../StdAfx.h"

#include "frame_shader.h"

#include "../../common/exception/exceptions.h"
#include "../../common/gl/utility.h"

#include <Glee.h>

#include <boost/noncopyable.hpp>

#include <unordered_map>

namespace caspar { namespace core {
	
class shader_program : boost::noncopyable
{
public:
	shader_program() : program_(0){}
	shader_program(shader_program&& other) : program_(other.program_){}
	shader_program& operator=(shader_program&& other) 
	{
		program_ = other.program_; 
		other.program_ = 0; 
		return *this;
	}

	shader_program(const std::string& fragment_source_str) : program_(0)
	{
		GLint success;

		try
		{		
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
				BOOST_THROW_EXCEPTION(common::gl::gl_error() << msg_info(str.str()));
			}
			
			program_ = glCreateProgramObjectARB();

			GL(glAttachObjectARB(program_, fragmemt_shader));

			GL(glDeleteObjectARB(fragmemt_shader));

			GL(glLinkProgramARB(program_));

			GL(glGetObjectParameterivARB(program_, GL_OBJECT_LINK_STATUS_ARB, &success));
			if (success == GL_FALSE)
			{
				char info[2048];
				GL(glGetInfoLogARB(program_, sizeof(info), 0, info));
				GL(glDeleteObjectARB(program_));
				std::stringstream str;
				str << "Failed to link shader program:" << std::endl << info << std::endl;
				BOOST_THROW_EXCEPTION(common::gl::gl_error() << msg_info(str.str()));
			}
			GL(glUseProgramObjectARB(program_));
			glUniform1i(glGetUniformLocation(program_, "plane[0]"), 0);
			glUniform1i(glGetUniformLocation(program_, "plane[1]"), 1);
			glUniform1i(glGetUniformLocation(program_, "plane[2]"), 2);
			glUniform1i(glGetUniformLocation(program_, "plane[3]"), 3);
		}
		catch(...)
		{
			CASPAR_LOG_CURRENT_EXCEPTION();
			throw;
		}
	}

	~shader_program()
	{
		glDeleteProgram(program_);
	}

	void use()
	{	
		GL(glUseProgramObjectARB(program_));		
	}
	
private:
	GLuint program_;
};
typedef std::shared_ptr<shader_program> shader_program_ptr;

struct frame_shader::implementation
{
	implementation(const video_format_desc& format_desc) 
		: current_(pixel_format::invalid), format_desc_(format_desc)
	{
		std::string common = 
			"uniform sampler2D	plane[4];											"
			"uniform vec4		plane_size[4];										"
																				
			// NOTE: YCbCr, ITU-R, http://www.intersil.com/data/an/an9717.pdf		
			// TODO: Support for more yuv formats might be needed.					
			"vec4 ycbcra_to_bgra(float y, float cb, float cr, float a)				"
			"{																		"
			"	vec4 color;															"
			"																		"
			"	cb -= 0.5;															"
			"	cr -= 0.5;															"
			"	y = 1.164*(y-0.0625);												"
			"																		"
			"	color.r = y + 1.596 * cr;											"
			"	color.g = y - 0.813 * cr - 0.337633 * cb;							"
			"	color.b = y + 2.017 * cb;											"
			"	color.a = a;														"
			"																		"
			"	return color;														"
			"}																		";
			
		shaders_[pixel_format::abgr] = common +

			"void main()															"
			"{																		"
				"vec4 abgr = texture2D(plane[0], gl_TexCoord[0].st);				"
				"gl_FragColor = abgr.argb * gl_Color;								"
			"}																		";
		
		shaders_[pixel_format::argb] = common +

			"void main()															"	
			"{																		"
				"vec4 argb = texture2D(plane[0], gl_TexCoord[0].st);				"
				"gl_FragColor = argb.grab * gl_Color;								"	
			"}																		";
		
		shaders_[pixel_format::bgra] = common +

			"void main()															"
			"{																		"
				"vec4 bgra = texture2D(plane[0], gl_TexCoord[0].st);				"
				"gl_FragColor = bgra.rgba * gl_Color;								"
			"}																		";
		
		shaders_[pixel_format::rgba] = common +

			"void main()															"
			"{																		"
				"vec4 rgba = texture2D(plane[0], gl_TexCoord[0].st);				"
				"gl_FragColor = rgba.bgra * gl_Color;								"
			"}																		";
		
		shaders_[pixel_format::ycbcr] = common +

			"void main()															"
			"{																		"
				"float y  = texture2D(plane[0], gl_TexCoord[0].st).r;				"
				"float cb = texture2D(plane[1], gl_TexCoord[0].st).r;				"
				"float cr = texture2D(plane[2], gl_TexCoord[0].st).r;				"
				"float a = 1.0;														"	
				"gl_FragColor = ycbcra_to_bgra(y, cb, cr, a) * gl_Color;			"
			"}																		";
		
		shaders_[pixel_format::ycbcra] = common +

			"void main()															"
			"{																		"
				"float y  = texture2D(plane[0], gl_TexCoord[0].st).r;				"
				"float cb = texture2D(plane[1], gl_TexCoord[0].st).r;				"
				"float cr = texture2D(plane[2], gl_TexCoord[0].st).r;				"
				"float a  = texture2D(plane[3], gl_TexCoord[0].st).r;				"
				"gl_FragColor = ycbcra_to_bgra(y, cb, cr, a) * gl_Color;			"
			"}																		";
	}

	void use(const pixel_format_desc& desc)
	{
		set_pixel_format(desc.pix_fmt);
	}

	void set_pixel_format(pixel_format::type format)
	{
		if(current_ == format)
			return;
		current_ = format;
		shaders_[format].use();
	}

	//void set_size(pixel_format_desc& desc)
	//{
	//	for(int n = 0; n < 4; ++n)
	//	{
	//		std::string name = std::string("plane_size[") + boost::lexical_cast<std::string>(n) + "]";
	//		GL(glUniform4f(shaders_[current_].get_location(name), 
	//			static_cast<float>(desc.planes[n].width), 
	//			static_cast<float>(desc.planes[n].height),
	//			1.0f/static_cast<float>(desc.planes[n].width),
	//			1.0f/static_cast<float>(desc.planes[n].height)));
	//	}
	//}	

	video_format_desc format_desc_;
	pixel_format::type current_;
	std::unordered_map<pixel_format::type, shader_program> shaders_;
};

frame_shader::frame_shader(const video_format_desc& format_desc) : impl_(new implementation(format_desc)){}
void frame_shader::use(const pixel_format_desc& desc){impl_->use(desc);}

}}