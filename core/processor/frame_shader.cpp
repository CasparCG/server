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

	shader_program(const std::string& vertex_source_str, const std::string& fragment_source_str) : program_(0)
	{
		GLint success;

		try
		{		
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
				BOOST_THROW_EXCEPTION(common::gl::gl_error() << msg_info(str.str()));
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
				BOOST_THROW_EXCEPTION(common::gl::gl_error() << msg_info(str.str()));
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
		std::string common_vertex = 
			"uniform sampler2D	plane[4];											"
			"uniform vec4		plane_size[2];										"
			"																		"
			"void main()															"
			"{																		"
			"	vec2 t0 = gl_MultiTexCoord0.xy + plane_size[0].zw*0.5;				"
			"	vec4 x0 = vec4(-1.0, 0.0, 1.0, 2.0)*plane_size[0].z;				"
			"	vec4 y0 = vec4(-1.0, 0.0, 1.0, 2.0)*plane_size[0].w;				"
			"	gl_TexCoord[0] = t0.xyxy + vec4(x0.x, y0.x, x0.y, y0.y);			"
			"	gl_TexCoord[1] = t0.xyxy + vec4(x0.z, y0.x, x0.w, y0.y); 			"
			"	gl_TexCoord[2] = t0.xyxy + vec4(x0.x, y0.z, x0.y, y0.w);			"
			"	gl_TexCoord[3] = t0.xyxy + vec4(x0.z, y0.z, x0.w, y0.w); 			"
			"																		"
			"	vec2 t1 = gl_MultiTexCoord0.xy + plane_size[1].zw*0.5;				"
			"	vec4 x1 = vec4(-1.0, 0.0, 1.0, 2.0)*plane_size[1].z;				"
			"	vec4 y1 = vec4(-1.0, 0.0, 1.0, 2.0)*plane_size[1].w;				"
			"	gl_TexCoord[4] = t1.xyxy + vec4(x1.x, y1.x, x1.y, y1.y);			"
			"	gl_TexCoord[5] = t1.xyxy + vec4(x1.z, y1.x, x1.w, y1.y); 			"
			"	gl_TexCoord[6] = t1.xyxy + vec4(x1.x, y1.z, x1.y, y1.w);			"
			"	gl_TexCoord[7] = t1.xyxy + vec4(x1.z, y1.z, x1.w, y1.w); 			"
			"																		"
			"	gl_FrontColor = gl_Color;											"
			"	gl_Position = ftransform();											"
			"}																		";

		std::string common_fragment = 
			"uniform sampler2D	plane[4];											"
			"uniform vec4		plane_size[2];										"
																				
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
			"vec4 texture2DNearest(sampler2D sampler, vec4 uv0, vec4 uv1, vec4 uv2, vec4 uv3, vec4 size)"
			"{																		"
			"	return texture2D(sampler, uv0.zw);									"
			"}																		"
			"																		"
			"vec4 texture2DBilinear(sampler2D sampler, vec4 uv0, vec4 uv1, vec4 uv2, vec4 uv3, vec4 size)"
			"{																		"
			"	vec2 f = fract(uv0*size.xy);										"
			"																		"
			"	vec4 t0 = texture2D(sampler, uv0.zw);								"
			"	vec4 t1 = texture2D(sampler, uv1.xw);								"
			"	vec4 t2 = texture2D(sampler, uv2.zy);								"
			"	vec4 t3 = texture2D(sampler, uv3.xy);								"
			"																		"
			"	vec4 x0 = mix(t0, t1, f.x);											"
			"	vec4 x1 = mix(t2, t3, f.x);											"
			"	return mix(x0, x1, f.y);											"
			"}																		"
			"																		"
			"vec4 computeWeights(float x)											"
			"{																		"
			"	vec4 x1 = x*vec4(1.0, 1.0, -1.0, -1.0) + vec4(1.0, 0.0, 1.0, 2.0);	"
			"	vec4 x2 = x1*x1;													"
			"	vec4 x3 = x2*x1;													"
			"																		"
			"	const float A = -0.75;												"	
			"	vec4 w;																"
			"	w  = x3 * vec2( A,       A+2.0 ).xyyx;								"
			"	w += x2 * vec2(-5.0*A, -(A+3.0)).xyyx;								"
			"	w += x1 * vec2( 8.0*A,       0 ).xyyx;								"
			"	w +=      vec2(-4.0*A,     1.0 ).xyyx;								"
			"	return w;															"
			"}																		"
			"																		"
			"vec4 cubicFilter(vec4 w, vec4 c0, vec4 c1, vec4 c2, vec4 c3)			"
			"{																		"
			"	return c0*w[0] + c1*w[1] + c2*w[2] + c3*w[3];						"
			"}																		"
			"																		"
			"vec4 texture2DBicubic(sampler2D sampler, vec4 uv0, vec4 uv1, vec4 uv2, vec4 uv3, vec4 size)"
			"{																		"
			"	vec2 f = fract(uv0*size.xy);										"
			"																		"
			"	vec4 w = computeWeights(f.x);										"
			"	vec4 x0 = cubicFilter(w, texture2D(sampler, uv0.xy),				"
			"				  			 texture2D(sampler, uv0.zy),				"
			"				  			 texture2D(sampler, uv1.xy),				"
			"				  			 texture2D(sampler, uv1.zy));				"
			"	vec4 x1 = cubicFilter(w, texture2D(sampler, uv0.xw),				"
			"							 texture2D(sampler, uv0.zw),				"
			"							 texture2D(sampler, uv1.xw),				"
			"							 texture2D(sampler, uv1.zw));				"
			"	vec4 x2 = cubicFilter(w, texture2D(sampler, uv2.xy),				"
			"							 texture2D(sampler, uv2.zy),				"
			"							 texture2D(sampler, uv3.xy),				"
			"							 texture2D(sampler, uv3.zy));				"
			"	vec4 x3 = cubicFilter(w, texture2D(sampler, uv2.xw),				"
			"							 texture2D(sampler, uv2.zw),				"
			"							 texture2D(sampler, uv3.xw),				"
			"							 texture2D(sampler, uv3.zw));				"
			"																		"
			"	w = computeWeights(f.y);											"
			"	return cubicFilter(w, x0, x1, x2, x3);								"
			"}																		";		
			
		shaders_[pixel_format::abgr] = shader_program(common_vertex, common_fragment +

			"void main()															"
			"{																		"
			"	vec4 abgr = texture2DBicubic(plane[0], gl_TexCoord[0], gl_TexCoord[1], gl_TexCoord[2], gl_TexCoord[3], plane_size[0]);"
			"	gl_FragColor = abgr.argb * gl_Color;								"
			"}																		");
		
		shaders_[pixel_format::argb]= shader_program(common_vertex, common_fragment +

			"void main()															"	
			"{																		"
			"	vec4 argb = texture2DBicubic(plane[0], gl_TexCoord[0], gl_TexCoord[1], gl_TexCoord[2], gl_TexCoord[3],  plane_size[0]);"
			"	gl_FragColor = argb.grab * gl_Color;								"	
			"}																		");
		
		shaders_[pixel_format::bgra]= shader_program(common_vertex, common_fragment +

			"void main()															"
			"{																		"
			"	vec4 bgra = texture2DBicubic(plane[0], gl_TexCoord[0], gl_TexCoord[1], gl_TexCoord[2], gl_TexCoord[3],  plane_size[0]);"
			"	gl_FragColor = bgra.rgba * gl_Color;								"
			"}																		");
		
		shaders_[pixel_format::rgba] = shader_program(common_vertex, common_fragment +

			"void main()															"
			"{																		"
			"	vec4 rgba = texture2DBicubic(plane[0], gl_TexCoord[0], gl_TexCoord[1], gl_TexCoord[2], gl_TexCoord[3],  plane_size[0]);"
			"	gl_FragColor = rgba.bgra * gl_Color;								"
			"}																		");
		
		shaders_[pixel_format::ycbcr] = shader_program(common_vertex, common_fragment +

			"void main()															"
			"{																		"
			"	float y  = texture2DBicubic(plane[0], gl_TexCoord[0], gl_TexCoord[1], gl_TexCoord[2], gl_TexCoord[3],  plane_size[0]).r;"
			"	float cb = texture2DBicubic(plane[1], gl_TexCoord[4], gl_TexCoord[5], gl_TexCoord[6], gl_TexCoord[7],  plane_size[1]).r;"
			"	float cr = texture2DBicubic(plane[2], gl_TexCoord[4], gl_TexCoord[5], gl_TexCoord[6], gl_TexCoord[7],  plane_size[1]).r;"
			"	float a = 1.0;														"	
			"	gl_FragColor = ycbcra_to_bgra(y, cb, cr, a) * gl_Color;			"
			"}																		");
		
		shaders_[pixel_format::ycbcra] = shader_program(common_vertex, common_fragment +

			"void main()															"
			"{																		"
			"	float y  = texture2DBicubic(plane[0], gl_TexCoord[0], gl_TexCoord[1], gl_TexCoord[2], gl_TexCoord[3],  plane_size[0]).r;"
			"	float cb = texture2DBicubic(plane[1], gl_TexCoord[4], gl_TexCoord[5], gl_TexCoord[6], gl_TexCoord[7],  plane_size[1]).r;"
			"	float cr = texture2DBicubic(plane[2], gl_TexCoord[4], gl_TexCoord[5], gl_TexCoord[6], gl_TexCoord[7],  plane_size[1]).r;"
			"	float a  = texture2DBicubic(plane[3], gl_TexCoord[0], gl_TexCoord[1], gl_TexCoord[2], gl_TexCoord[3],  plane_size[0]).r;"
			"	gl_FragColor = ycbcra_to_bgra(y, cb, cr, a) * gl_Color;				"
			"}																		");
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
		for(size_t n = 0; n < planes.size() && n < 2; ++n)
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