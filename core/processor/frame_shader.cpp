#include "../StdAfx.h"

#include "frame_shader.h"

#include "../../common/exception/exceptions.h"
#include "../../common/gl/utility.h"
#include "../../common/gl/shader_program.h"

#include <Glee.h>

#include <boost/noncopyable.hpp>

#include <unordered_map>

namespace caspar { namespace core {
		
GLubyte progressive_pattern[] = {
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xFF, 0xff, 0xff, 0xff, 0xff, 0xff,	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
	
GLubyte upper_pattern[] = {
	0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00,	0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00,
	0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00,	0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00,
	0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00,	0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00,
	0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00,	0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00,
	0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00,	0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00,
	0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00,	0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00,
	0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00,	0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00,
	0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00,	0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00};
		
GLubyte lower_pattern[] = {
	0x00, 0x00, 0x00, 0x00,	0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 
	0x00, 0x00, 0x00, 0x00,	0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 
	0x00, 0x00, 0x00, 0x00,	0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 
	0x00, 0x00, 0x00, 0x00,	0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00,	0xff, 0xff, 0xff, 0xff,
	0x00, 0x00, 0x00, 0x00,	0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00,	0xff, 0xff, 0xff, 0xff,
	0x00, 0x00, 0x00, 0x00,	0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00,	0xff, 0xff, 0xff, 0xff,
	0x00, 0x00, 0x00, 0x00,	0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00,	0xff, 0xff, 0xff, 0xff,
	0x00, 0x00, 0x00, 0x00,	0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00,	0xff, 0xff, 0xff, 0xff};
	
struct frame_shader::implementation
{
	implementation(const video_format_desc& format_desc) : current_(pixel_format::invalid), format_desc_(format_desc), alpha_(1.0)
	{
		transform_stack_.push(shader_transform());
		std::string common_vertex = 
			"void main()															"
			"{																		"
			"	gl_TexCoord[0] = gl_MultiTexCoord0;									"
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
			"																		";
			
		shaders_[pixel_format::abgr] = common::gl::shader_program(common_vertex, common_fragment +

			"void main()															"
			"{																		"
			"	vec4 abgr = texture2D(plane[0], gl_TexCoord[0].st);"
			"	gl_FragColor = abgr.argb * gl_Color;								"
			"}																		");
		
		shaders_[pixel_format::argb]= common::gl::shader_program(common_vertex, common_fragment +

			"void main()															"	
			"{																		"
			"	vec4 argb = texture2D(plane[0], gl_TexCoord[0].st);"
			"	gl_FragColor = argb.grab * gl_Color;								"	
			"}																		");
		
		shaders_[pixel_format::bgra]= common::gl::shader_program(common_vertex, common_fragment +

			"void main()															"
			"{																		"
			"	vec4 bgra = texture2D(plane[0], gl_TexCoord[0].st);"
			"	gl_FragColor = bgra.rgba * gl_Color;								"
			"}																		");
		
		shaders_[pixel_format::rgba] = common::gl::shader_program(common_vertex, common_fragment +

			"void main()															"
			"{																		"
			"	vec4 rgba = texture2D(plane[0], gl_TexCoord[0].st);					"
			"	gl_FragColor = rgba.bgra * gl_Color;								"
			"}																		");
		
		shaders_[pixel_format::ycbcr] = common::gl::shader_program(common_vertex, common_fragment +

			"void main()															"
			"{																		"
			"	float y  = texture2D(plane[0], gl_TexCoord[0].st).r;"
			"	float cb = texture2D(plane[1], gl_TexCoord[0].st).r;"
			"	float cr = texture2D(plane[2], gl_TexCoord[0].st).r;"
			"	float a = 1.0;														"	
			"	gl_FragColor = ycbcra_to_bgra(y, cb, cr, a) * gl_Color;			"
			"}																		");
		
		shaders_[pixel_format::ycbcra] = common::gl::shader_program(common_vertex, common_fragment +

			"void main()															"
			"{																		"
			"	float y  = texture2D(plane[0], gl_TexCoord[0].st).r;"
			"	float cb = texture2D(plane[1], gl_TexCoord[0].st).r;"
			"	float cr = texture2D(plane[2], gl_TexCoord[0].st).r;"
			"	float a  = texture2D(plane[3], gl_TexCoord[0].st).r;"
			"	gl_FragColor = ycbcra_to_bgra(y, cb, cr, a) * gl_Color;				"
			"}																		");
	}

	void begin(const shader_transform& transform)
	{
		transform_stack_.push(transform);

		glPushMatrix();
		alpha_ *= transform_stack_.top().alpha;
		glColor4d(1.0, 1.0, 1.0, alpha_);
		glTranslated(transform_stack_.top().pos.get<0>()*2.0, transform_stack_.top().pos.get<1>()*2.0, 0.0);

		if(transform.mode == video_mode::upper)
			glPolygonStipple(upper_pattern);
		else if(transform.mode == video_mode::lower)
			glPolygonStipple(lower_pattern);
		else
			glPolygonStipple(progressive_pattern);
	}
		
	void render(const pixel_format_desc& desc)
	{
		set_pixel_format(desc.pix_fmt);

		auto t = transform_stack_.top();
		glBegin(GL_QUADS);
			glTexCoord2d(0.0, 0.0); glVertex2d(-1.0, -1.0);
			glTexCoord2d(1.0, 0.0); glVertex2d( 1.0, -1.0);
			glTexCoord2d(1.0, 1.0); glVertex2d( 1.0,  1.0);
			glTexCoord2d(0.0, 1.0); glVertex2d(-1.0,  1.0);
		glEnd();
	}

	void end()
	{
		alpha_ /= transform_stack_.top().alpha;
		glPopMatrix();
		transform_stack_.pop();
	}

	void set_pixel_format(pixel_format::type format)
	{
		if(current_ == format)
			return;
		current_ = format;
		shaders_[format].use();
	}
		 
	double alpha_;
	std::stack<shader_transform> transform_stack_;

	video_format_desc format_desc_;
	pixel_format::type current_;
	std::unordered_map<pixel_format::type, common::gl::shader_program> shaders_;
};

frame_shader::frame_shader(const video_format_desc& format_desc) : impl_(new implementation(format_desc)){}
void frame_shader::begin(const shader_transform& transform) {impl_->begin(transform);}
void frame_shader::render(const pixel_format_desc& desc){impl_->render(desc);}
void frame_shader::end(){impl_->end();}
}}