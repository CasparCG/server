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

	GLuint program() { return program_; }
	
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
			"}																		"
			"																		"
			"vec4 texture2DBilinear(sampler2D sampler, vec2 uv, vec4 size)			"
			"{																		"
			"	vec4 x0 = texture2D(sampler, uv + size.zw * vec2(0.0, 0.0));		"
			"	vec4 x1 = texture2D(sampler, uv + size.zw * vec2(1.0, 0.0));		"
			"	vec4 x2 = texture2D(sampler, uv + size.zw * vec2(0.0, 1.0));		"
			"	vec4 x3 = texture2D(sampler, uv + size.zw * vec2(1.0, 1.0));		"
			"	vec2 f  = fract(uv.xy * size.xy );									"
			"	vec4 y0 = mix(x0, x1, f.x);											"
			"	vec4 y1 = mix(x2, x3, f.x);											"
			"	return mix(y0, y1, f.y);											"
			"}																		"
			"																					"
			"vec4 computeWeights(float x)														"
			"{																					"
			"	vec4 x1 = x*vec4(1.0, 1.0, -1.0, -1.0) + vec4(1.0, 0.0, 1.0, 2.0);				"
			"	vec4 x2 = x1*x1;																"
			"	vec4 x3 = x2*x1;																"
			"																					"
			"	const float A = -0.75;															"	
			"	vec4 w;																			"
			"	w =  x3 * vec2(  A,      A+2.0).xyyx;											"
			"	w += x2 * vec2( -5.0*A, -(A+3.0)).xyyx;											"
			"	w += x1 * vec2(  8.0*A,  0).xyyx;												"
			"	w +=      vec2( -4.0*A,  1.0).xyyx;												"
			"	return w;																		"
			"}																					"
			"																					"
			"vec4 cubicFilter(vec4 w, vec4 c0, vec4 c1, vec4 c2, vec4 c3)						"
			"{																					"
			"	return c0*w[0] + c1*w[1] + c2*w[2] + c3*w[3];									"
			"}																					"
			"																					"
			"vec4 texture2DBicubic(sampler2D sampler, vec2 uv, vec4 size)						"
			"{																					"
			"	vec2 f = fract(uv*size.xy);														"
			"																					"
			"	vec4 w = computeWeights(f.x);													"
			"	vec4 t0 = cubicFilter(w,	texture2D(sampler, uv + vec2(-1.0, -1.0) * size.zw),"
			"				  				texture2D(sampler, uv + vec2( 0.0, -1.0) * size.zw),"
			"				  				texture2D(sampler, uv + vec2( 1.0, -1.0) * size.zw),"
			"				  				texture2D(sampler, uv + vec2( 2.0, -1.0) * size.zw));"
			"	vec4 t1 = cubicFilter(w,	texture2D(sampler, uv + vec2(-1.0,  0.0) * size.zw),"
			"								texture2D(sampler, uv + vec2( 0.0,  0.0) * size.zw),"
			"								texture2D(sampler, uv + vec2( 1.0,  0.0) * size.zw),"
			"								texture2D(sampler, uv + vec2( 2.0,  0.0) * size.zw));"
			"	vec4 t2 = cubicFilter(w,	texture2D(sampler, uv + vec2(-1.0,  1.0) * size.zw),"
			"								texture2D(sampler, uv + vec2( 0.0,  1.0) * size.zw),"
			"								texture2D(sampler, uv + vec2( 1.0,  1.0) * size.zw),"
			"								texture2D(sampler, uv + vec2( 2.0,  1.0) * size.zw));"
			"	vec4 t3 = cubicFilter(w,	texture2D(sampler, uv + vec2(-1.0,  2.0) * size.zw),"
			"								texture2D(sampler, uv + vec2( 0.0,  2.0) * size.zw),"
			"								texture2D(sampler, uv + vec2( 1.0,  2.0) * size.zw),"
			"								texture2D(sampler, uv + vec2( 2.0,  2.0) * size.zw));"
			"																					"
			"	w = computeWeights(f.y);														"
			"	return cubicFilter(w, t0, t1, t2, t3);											"
			"}																					";
			
		shaders_[pixel_format::abgr] = common +

			"void main()															"
			"{																		"
			"	vec4 abgr = texture2DBicubic(plane[0], gl_TexCoord[0].st+plane_size[0].zw*0.5, plane_size[0]);"
				"gl_FragColor = abgr.argb * gl_Color;								"
			"}																		";
		
		shaders_[pixel_format::argb] = common +

			"void main()															"	
			"{																		"
				"vec4 argb = texture2DBicubic(plane[0], gl_TexCoord[0].st+plane_size[0].zw*0.5, plane_size[0]);"
				"gl_FragColor = argb.grab * gl_Color;								"	
			"}																		";
		
		shaders_[pixel_format::bgra] = common +

			"void main()															"
			"{																		"
				"vec4 bgra = texture2DBicubic(plane[0], gl_TexCoord[0].st+plane_size[0].zw*0.5, plane_size[0]);"
				"gl_FragColor = bgra.rgba * gl_Color;								"
			"}																		";
		
		shaders_[pixel_format::rgba] = common +

			"void main()															"
			"{																		"
				"vec4 rgba = texture2DBicubic(plane[0], gl_TexCoord[0].st+plane_size[0].zw*0.5, plane_size[0]);"
				"gl_FragColor = rgba.bgra * gl_Color;								"
			"}																		";
		
		shaders_[pixel_format::ycbcr] = common +

			"void main()															"
			"{																		"
				"float y  = texture2DBicubic(plane[0], gl_TexCoord[0].st+plane_size[0].zw*0.5, plane_size[0]).r;"
				"float cb = texture2DBicubic(plane[1], gl_TexCoord[0].st+plane_size[1].zw*0.5, plane_size[1]).r;"
				"float cr = texture2DBicubic(plane[2], gl_TexCoord[0].st+plane_size[2].zw*0.5, plane_size[2]).r;"
				"float a = 1.0;														"	
				"gl_FragColor = ycbcra_to_bgra(y, cb, cr, a) * gl_Color;			"
			"}																		";
		
		shaders_[pixel_format::ycbcra] = common +

			"void main()															"
			"{																		"
				"float y  = texture2DBicubic(plane[0], gl_TexCoord[0].st+plane_size[0].zw*0.5, plane_size[0]).r;"
				"float cb = texture2DBicubic(plane[1], gl_TexCoord[0].st+plane_size[1].zw*0.5, plane_size[1]).r;"
				"float cr = texture2DBicubic(plane[2], gl_TexCoord[0].st+plane_size[2].zw*0.5, plane_size[2]).r;"
				"float a  = texture2DBicubic(plane[3], gl_TexCoord[0].st+plane_size[3].zw*0.5, plane_size[3]).r;"
				"gl_FragColor = ycbcra_to_bgra(y, cb, cr, a) * gl_Color;			"
			"}																		";
	}

	void use(const pixel_format_desc& desc)
	{
		set_pixel_format(desc.pix_fmt);
		set_planes(desc.planes);
	}

	void set_pixel_format(pixel_format::type format)
	{
		if(current_ == format)
			return;
		current_ = format;
		shaders_[format].use();
	}

	void set_planes(const std::vector<pixel_format_desc::plane>& planes)
	{
		for(size_t n = 0; n < planes.size(); ++n)
			glUniform4f(glGetUniformLocation(shaders_[current_].program(), std::string("plane_size[" + boost::lexical_cast<std::string>(n) + "]").c_str()), 
				static_cast<float>(planes[n].width), static_cast<float>(planes[n].height), 1.0f/static_cast<float>(planes[n].width), 1.0f/static_cast<float>(planes[n].height));
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