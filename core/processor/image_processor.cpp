#include "../StdAfx.h"

#include "image_processor.h"

#include "../../common/exception/exceptions.h"
#include "../../common/gl/utility.h"
#include "../../common/gl/shader_program.h"
#include "../../common/gl/frame_buffer_object.h"

#include <Glee.h>
#include <SFML/Window.hpp>

#include <boost/noncopyable.hpp>

#include <unordered_map>

struct ogl_context
{
	ogl_context(){context_.SetActive(true);};
	sf::Context context_;
};

namespace caspar { namespace core {

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
	
struct image_processor::implementation : boost::noncopyable
{
	implementation(const video_format_desc& format_desc) : fbo_(format_desc.width, format_desc.height), current_(pixel_format::invalid), reading_(create_reading())
	{
		alpha_stack_.push(1.0);
		GL(glEnable(GL_POLYGON_STIPPLE));
		GL(glEnable(GL_TEXTURE_2D));
		GL(glEnable(GL_BLEND));
		GL(glDisable(GL_DEPTH_TEST));
		GL(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));			
		GL(glViewport(0, 0, format_desc.width, format_desc.height));
		reading_->unmap();

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
			"	cb -= 0.5;															"
			"	cr -= 0.5;															"
			"	y = 1.164*(y-0.0625);												"
			"																		"
			"	vec4 color;															"
			"	color.r = y + 1.596 * cr;											"
			"	color.g = y - 0.813 * cr - 0.337633 * cb;							"
			"	color.b = y + 2.017 * cb;											"
			"	color.a = a;														"
			"																		"
			"	return color;														"
			"}																		"			
			"																		";
			
		shaders_[pixel_format::abgr] = gl::shader_program(common_vertex, common_fragment +

			"void main()															"
			"{																		"
			"	vec4 abgr = texture2D(plane[0], gl_TexCoord[0].st);"
			"	gl_FragColor = abgr.argb * gl_Color;								"
			"}																		");
		
		shaders_[pixel_format::argb]= gl::shader_program(common_vertex, common_fragment +

			"void main()															"	
			"{																		"
			"	vec4 argb = texture2D(plane[0], gl_TexCoord[0].st);"
			"	gl_FragColor = argb.grab * gl_Color;								"	
			"}																		");
		
		shaders_[pixel_format::bgra]= gl::shader_program(common_vertex, common_fragment +

			"void main()															"
			"{																		"
			"	vec4 bgra = texture2D(plane[0], gl_TexCoord[0].st);"
			"	gl_FragColor = bgra.rgba * gl_Color;								"
			"}																		");
		
		shaders_[pixel_format::rgba] = gl::shader_program(common_vertex, common_fragment +

			"void main()															"
			"{																		"
			"	vec4 rgba = texture2D(plane[0], gl_TexCoord[0].st);					"
			"	gl_FragColor = rgba.bgra * gl_Color;								"
			"}																		");
		
		shaders_[pixel_format::ycbcr] = gl::shader_program(common_vertex, common_fragment +

			"void main()															"
			"{																		"
			"	float y  = texture2D(plane[0], gl_TexCoord[0].st).r;"
			"	float cb = texture2D(plane[1], gl_TexCoord[0].st).r;"
			"	float cr = texture2D(plane[2], gl_TexCoord[0].st).r;"
			"	float a = 1.0;														"	
			"	gl_FragColor = ycbcra_to_bgra(y, cb, cr, a) * gl_Color;			"
			"}																		");
		
		shaders_[pixel_format::ycbcra] = gl::shader_program(common_vertex, common_fragment +

			"void main()															"
			"{																		"
			"	float y  = texture2D(plane[0], gl_TexCoord[0].st).r;"
			"	float cb = texture2D(plane[1], gl_TexCoord[0].st).r;"
			"	float cr = texture2D(plane[2], gl_TexCoord[0].st).r;"
			"	float a  = texture2D(plane[3], gl_TexCoord[0].st).r;"
			"	gl_FragColor = ycbcra_to_bgra(y, cb, cr, a) * gl_Color;				"
			"}																		");
	}

	void begin(const image_transform& transform)
	{
		alpha_stack_.push(alpha_stack_.top()*transform.alpha);

		glPushMatrix();
		glColor4d(1.0, 1.0, 1.0, alpha_stack_.top());
		glTranslated(transform.pos.get<0>()*2.0, transform.pos.get<1>()*2.0, 0.0);

		if(transform.mode == video_mode::upper)
			glPolygonStipple(upper_pattern);
		else if(transform.mode == video_mode::lower)
			glPolygonStipple(lower_pattern);
		else
			glPolygonStipple(progressive_pattern);
	}
		
	void render(const pixel_format_desc& desc, std::vector<gl::pbo>& pbos)
	{
		set_pixel_format(desc.pix_fmt);

		for(size_t n = 0; n < pbos.size(); ++n)
		{
			glActiveTexture(GL_TEXTURE0+n);
			pbos[n].unmap_write();
		}

		glBegin(GL_QUADS);
			glTexCoord2d(0.0, 0.0); glVertex2d(-1.0, -1.0);
			glTexCoord2d(1.0, 0.0); glVertex2d( 1.0, -1.0);
			glTexCoord2d(1.0, 1.0); glVertex2d( 1.0,  1.0);
			glTexCoord2d(0.0, 1.0); glVertex2d(-1.0,  1.0);
		glEnd();
	}

	void end()
	{
		alpha_stack_.pop();
		glPopMatrix();
	}

	void begin_pass()
	{
		GL(glClear(GL_COLOR_BUFFER_BIT));
	}

	void end_pass()
	{
		reading_ = create_reading();
		reading_->unmap(); // Start transfer from frame buffer to texture. End with map in next tick.
	}

	safe_ptr<read_frame> read()
	{
		reading_->map();
		return reading_;
	}

	void set_pixel_format(pixel_format::type format)
	{
		if(current_ == format)
			return;
		current_ = format;
		shaders_[format].use();
	}

	safe_ptr<read_frame> create_reading()
	{
		std::shared_ptr<read_frame> frame;
		if(!pool_.try_pop(frame))		
			frame = std::make_shared<read_frame>(fbo_.width(), fbo_.height());
		return safe_ptr<read_frame>(frame.get(), [=](read_frame*){pool_.push(frame);});
	}

	const ogl_context context_;
		 
	tbb::concurrent_bounded_queue<std::shared_ptr<read_frame>> pool_;

	std::stack<double> alpha_stack_;

	pixel_format::type current_;
	std::unordered_map<pixel_format::type, gl::shader_program> shaders_;
	gl::fbo fbo_;

	safe_ptr<read_frame> reading_;
};

image_processor::image_processor(const video_format_desc& format_desc) : format_desc_(format_desc){}
void image_processor::begin(const image_transform& transform) 
{
	if(!impl_)
		impl_.reset(new implementation(format_desc_));
	impl_->begin(transform);
}
void image_processor::process(const pixel_format_desc& desc, std::vector<gl::pbo>& pbos)
{
	if(!impl_)
		impl_.reset(new implementation(format_desc_));
	impl_->render(desc, pbos);
}
void image_processor::end()
{
	if(!impl_)
		impl_.reset(new implementation(format_desc_));
	impl_->end();
}
void image_processor::begin_pass()
{
	if(!impl_)
		impl_.reset(new implementation(format_desc_));
	impl_->begin_pass();
}
void image_processor::end_pass()
{
	if(!impl_)
		impl_.reset(new implementation(format_desc_));
	impl_->end_pass();
}
safe_ptr<read_frame> image_processor::read()
{
	if(!impl_)
		impl_.reset(new implementation(format_desc_));
	return impl_->read();
}
}}