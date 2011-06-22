/*
* copyright (c) 2010 Sveriges Television AB <info@casparcg.com>
*
*  This file is part of CasparCG.
*
*    CasparCG is free software: you can redistribute it and/or modify
*    it under the terms of the GNU General Public License as published by
*    the Free Software Foundation, either version 3 of the License, or
*    (at your option) any later version.
*
*    CasparCG is distributed in the hope that it will be useful,
*    but WITHOUT ANY WARRANTY; without even the implied warranty of
*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*    GNU General Public License for more details.

*    You should have received a copy of the GNU General Public License
*    along with CasparCG.  If not, see <http://www.gnu.org/licenses/>.
*
*/
#include "../../stdafx.h"

#include "image_kernel.h"

#include <common/exception/exceptions.h>
#include <common/gl/gl_check.h>

#include <core/video_format.h>
#include <core/producer/frame/pixel_format.h>
#include <core/producer/frame/image_transform.h>

#include <boost/noncopyable.hpp>

#include <unordered_map>

namespace caspar { namespace core {

class shader_program : boost::noncopyable
{
	GLuint program_;
public:

	shader_program() : program_(0) {}
	shader_program(shader_program&& other) : program_(other.program_){other.program_ = 0;}
	shader_program(const std::string& vertex_source_str, const std::string& fragment_source_str) : program_(0)
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
			BOOST_THROW_EXCEPTION(caspar_exception() << msg_info(str.str()));
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
			BOOST_THROW_EXCEPTION(caspar_exception() << msg_info(str.str()));
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
			BOOST_THROW_EXCEPTION(caspar_exception() << msg_info(str.str()));
		}
		GL(glUseProgramObjectARB(program_));
		glUniform1i(glGetUniformLocation(program_, "plane[0]"), 0);
		glUniform1i(glGetUniformLocation(program_, "plane[1]"), 1);
		glUniform1i(glGetUniformLocation(program_, "plane[2]"), 2);
		glUniform1i(glGetUniformLocation(program_, "plane[3]"), 3);
		glUniform1i(glGetUniformLocation(program_, "plane[4]"), 4);
	}

	GLint get_location(const char* name)
	{
		GLint loc = glGetUniformLocation(program_, name);
		return loc;
	}

	shader_program& operator=(shader_program&& other) 
	{
		program_ = other.program_; 
		other.program_ = 0; 
		return *this;
	}

	~shader_program()
	{
		glDeleteProgram(program_);
	}

	void use()
	{	
		GL(glUseProgramObjectARB(program_));		
	}
};

GLubyte progressive_pattern[] = {
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,	0xff, 0xff, 0xFF, 0xff, 0xff, 0xff, 0xff, 0xff,	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
	
GLubyte upper_pattern[] = {
	0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00,	0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00,	0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00,	0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00,
	0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00,	0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00,	0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00,	0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00,
	0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00,	0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00,	0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00,	0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00,
	0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00,	0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00,	0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00,	0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00};
		
GLubyte lower_pattern[] = {
	0x00, 0x00, 0x00, 0x00,	0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00,	0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 
	0x00, 0x00, 0x00, 0x00,	0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00,	0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00,	0xff, 0xff, 0xff, 0xff,
	0x00, 0x00, 0x00, 0x00,	0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00,	0xff, 0xff, 0xff, 0xff,	0x00, 0x00, 0x00, 0x00,	0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00,	0xff, 0xff, 0xff, 0xff,
	0x00, 0x00, 0x00, 0x00,	0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00,	0xff, 0xff, 0xff, 0xff,	0x00, 0x00, 0x00, 0x00,	0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00,	0xff, 0xff, 0xff, 0xff};

struct image_kernel::implementation : boost::noncopyable
{	
	std::unordered_map<core::pixel_format::type, shader_program> shaders_;

public:
	std::unordered_map<core::pixel_format::type, shader_program>& shaders()
	{
		GL(glEnable(GL_POLYGON_STIPPLE));
		GL(glEnable(GL_BLEND));

		if(shaders_.empty())
		{
		std::string common_vertex = 
			"void main()															"
			"{																		"
			"	gl_TexCoord[0] = gl_MultiTexCoord0;									"
			"	gl_TexCoord[1] = gl_MultiTexCoord1;									"
			"	gl_FrontColor = gl_Color;											"
			"	gl_Position = ftransform();											"
			"}																		";

		std::string common_fragment = 
			"uniform sampler2D	plane[5];											"
			"uniform float		gain;												"
			"uniform bool		HD;													"
			"uniform bool		local_key;											"
			"uniform bool		layer_key;											"
																				
			// NOTE: YCbCr, ITU-R, http://www.intersil.com/data/an/an9717.pdf		
			// TODO: Support for more yuv formats might be needed.					
			"vec4 ycbcra_to_bgra_sd(float y, float cb, float cr, float a)			"
			"{																		"
			"	cb -= 0.5;															"
			"	cr -= 0.5;															"
			"	y = 1.164*(y-0.0625);												"
			"																		"
			"	vec4 color;															"
			"	color.r = y + 1.596 * cr;											"
			"	color.g = y - 0.813 * cr - 0.391 * cb;								"
			"	color.b = y + 2.018 * cb;											"
			"	color.a = a;														"
			"																		"
			"	return color;														"
			"}																		"			
			"																		"

			"vec4 ycbcra_to_bgra_hd(float y, float cb, float cr, float a)			"
			"{																		"
			"	cb -= 0.5;															"
			"	cr -= 0.5;															"
			"	y = 1.164*(y-0.0625);												"
			"																		"
			"	vec4 color;															"
			"	color.r = y + 1.793 * cr;											"
			"	color.g = y - 0.534 * cr - 0.213 * cb;								"
			"	color.b = y + 2.115 * cb;											"
			"	color.a = a;														"
			"																		"
			"	return color;														"
			"}																		"			
			"																		";
			
		shaders_[core::pixel_format::gray] = shader_program(common_vertex, common_fragment +

			"void main()															"
			"{																		"
			"	vec4 rgba = vec4(texture2D(plane[0], gl_TexCoord[0].st).rrr, 1.0);	"
			"	if(local_key)														"
			"		rgba.a *= texture2D(plane[3], gl_TexCoord[1].st).r;				"
			"	if(layer_key)														"
			"		rgba.a *= texture2D(plane[4], gl_TexCoord[1].st).r;				"
			"	gl_FragColor = rgba * gain;											"
			"}																		");

		shaders_[core::pixel_format::abgr] = shader_program(common_vertex, common_fragment +

			"void main()															"
			"{																		"
			"	vec4 abgr = texture2D(plane[0], gl_TexCoord[0].st);					"
			"	if(local_key)														"
			"		abgr.b *= texture2D(plane[3], gl_TexCoord[1].st).r;				"
			"	if(layer_key)														"
			"		abgr.b *= texture2D(plane[4], gl_TexCoord[1].st).r;				"
			"	gl_FragColor = abgr.argb * gain;									"
			"}																		");
		
		shaders_[core::pixel_format::argb]= shader_program(common_vertex, common_fragment +

			"void main()															"	
			"{																		"
			"	vec4 argb = texture2D(plane[0], gl_TexCoord[0].st);					"
			"	if(local_key)														"
			"		argb.b *= texture2D(plane[3], gl_TexCoord[1].st).r;				"
			"	if(layer_key)														"
			"		argb.b *= texture2D(plane[4], gl_TexCoord[1].st).r;				"
			"	gl_FragColor = argb.grab * gl_Color * gain;							"
			"}																		");
		
		shaders_[core::pixel_format::bgra]= shader_program(common_vertex, common_fragment +

			"void main()															"
			"{																		"
			"	vec4 bgra = texture2D(plane[0], gl_TexCoord[0].st);					"
			"	if(local_key)														"
			"		bgra.a *= texture2D(plane[3], gl_TexCoord[1].st).r;				"
			"	if(layer_key)														"
			"		bgra.a *= texture2D(plane[4], gl_TexCoord[1].st).r;				"
			"	gl_FragColor = bgra.rgba * gl_Color * gain;							"
			"}																		");
		
		shaders_[core::pixel_format::rgba] = shader_program(common_vertex, common_fragment +

			"void main()															"
			"{																		"
			"	vec4 rgba = texture2D(plane[0], gl_TexCoord[0].st);					"
			"	if(local_key)														"
			"		rgba.a *= texture2D(plane[3], gl_TexCoord[1].st).r;				"
			"	if(layer_key)														"
			"		rgba.a *= texture2D(plane[4], gl_TexCoord[1].st).r;				"
			"	gl_FragColor = rgba.bgra * gl_Color * gain;							"
			"}																		");
		
		shaders_[core::pixel_format::ycbcr] = shader_program(common_vertex, common_fragment +

			"void main()															"
			"{																		"
			"	float y  = texture2D(plane[0], gl_TexCoord[0].st).r;				"
			"	float cb = texture2D(plane[1], gl_TexCoord[0].st).r;				"
			"	float cr = texture2D(plane[2], gl_TexCoord[0].st).r;				"
			"	float a = 1.0;														"	
			"	if(local_key)														"
			"		a *= texture2D(plane[3], gl_TexCoord[1].st).r;					"
			"	if(layer_key)														"
			"		a *= texture2D(plane[4], gl_TexCoord[1].st).r;					"
			"	if(HD)																"
			"		gl_FragColor = ycbcra_to_bgra_hd(y, cb, cr, a) * gl_Color * gain;"
			"	else																"
			"		gl_FragColor = ycbcra_to_bgra_sd(y, cb, cr, a) * gl_Color * gain;"
			"}																		");
		
		shaders_[core::pixel_format::ycbcra] = shader_program(common_vertex, common_fragment +

			"void main()															"
			"{																		"
			"	float y  = texture2D(plane[0], gl_TexCoord[0].st).r;				"
			"	float cb = texture2D(plane[1], gl_TexCoord[0].st).r;				"
			"	float cr = texture2D(plane[2], gl_TexCoord[0].st).r;				"
			"	float a  = texture2D(plane[3], gl_TexCoord[0].st).r;				"
			"	if(local_key)														"
			"		a *= texture2D(plane[3], gl_TexCoord[1].st).r;					"
			"	if(layer_key)														"
			"		a *= texture2D(plane[4], gl_TexCoord[1].st).r;					"
			"	if(HD)																"
			"		gl_FragColor = ycbcra_to_bgra_hd(y, cb, cr, a) * gl_Color * gain;"
			"	else																"
			"		gl_FragColor = ycbcra_to_bgra_sd(y, cb, cr, a) * gl_Color * gain;"
			"}																		");
		}
		return shaders_;
	}
};

image_kernel::image_kernel() : impl_(new implementation()){}

void image_kernel::draw(size_t width, size_t height, const core::pixel_format_desc& pix_desc, const core::image_transform& transform, bool local_key, bool layer_key)
{
	if(transform.get_opacity() < 0.001)
		return;

	switch(transform.get_blend_mode())
	{
	case image_transform::screen:
		GL(glBlendFuncSeparate(GL_ONE, GL_ONE_MINUS_SRC_COLOR, GL_ONE, GL_ONE));
			break;
	default:
		GL(glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE));
	}

	GL(glEnable(GL_TEXTURE_2D));
	GL(glDisable(GL_DEPTH_TEST));	

	impl_->shaders()[pix_desc.pix_fmt].use();

	GL(glUniform1f(impl_->shaders()[pix_desc.pix_fmt].get_location("gain"), static_cast<GLfloat>(transform.get_gain())));
	GL(glUniform1i(impl_->shaders()[pix_desc.pix_fmt].get_location("HD"), pix_desc.planes.at(0).height > 700 ? 1 : 0));
	GL(glUniform1i(impl_->shaders()[pix_desc.pix_fmt].get_location("local_key"), local_key ? 1 : 0));
	GL(glUniform1i(impl_->shaders()[pix_desc.pix_fmt].get_location("layer_key"), layer_key ? 1 : 0));

	if(transform.get_mode() == core::video_mode::upper)
		glPolygonStipple(upper_pattern);
	else if(transform.get_mode() == core::video_mode::lower)
		glPolygonStipple(lower_pattern);
	else
		glPolygonStipple(progressive_pattern);
			
	GL(glColor4d(1.0, 1.0, 1.0, transform.get_opacity()));
	GL(glViewport(0, 0, width, height));
						
	auto m_p = transform.get_clip_translation();
	auto m_s = transform.get_clip_scale();
	double w = static_cast<double>(width);
	double h = static_cast<double>(height);

	GL(glEnable(GL_SCISSOR_TEST));
	GL(glScissor(static_cast<size_t>(m_p[0]*w), static_cast<size_t>(m_p[1]*h), static_cast<size_t>(m_s[0]*w), static_cast<size_t>(m_s[1]*h)));
			
	auto f_p = transform.get_fill_translation();
	auto f_s = transform.get_fill_scale();
			
	glBegin(GL_QUADS);
		glMultiTexCoord2d(GL_TEXTURE0, 0.0, 0.0); glMultiTexCoord2d(GL_TEXTURE1,  f_p[0]        ,  f_p[1]        );		glVertex2d( f_p[0]        *2.0-1.0,  f_p[1]        *2.0-1.0);
		glMultiTexCoord2d(GL_TEXTURE0, 1.0, 0.0); glMultiTexCoord2d(GL_TEXTURE1, (f_p[0]+f_s[0]),  f_p[1]        );		glVertex2d((f_p[0]+f_s[0])*2.0-1.0,  f_p[1]        *2.0-1.0);
		glMultiTexCoord2d(GL_TEXTURE0, 1.0, 1.0); glMultiTexCoord2d(GL_TEXTURE1, (f_p[0]+f_s[0]), (f_p[1]+f_s[1]));		glVertex2d((f_p[0]+f_s[0])*2.0-1.0, (f_p[1]+f_s[1])*2.0-1.0);
		glMultiTexCoord2d(GL_TEXTURE0, 0.0, 1.0); glMultiTexCoord2d(GL_TEXTURE1,  f_p[0]        , (f_p[1]+f_s[1]));		glVertex2d( f_p[0]        *2.0-1.0, (f_p[1]+f_s[1])*2.0-1.0);
	glEnd();
	GL(glDisable(GL_SCISSOR_TEST));	
}

}}