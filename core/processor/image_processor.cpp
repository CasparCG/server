#include "../StdAfx.h"

#include "image_processor.h"

#include "buffer/read_buffer.h"
#include "buffer/write_buffer.h"

#include <common/exception/exceptions.h>
#include <common/gl/utility.h>
#include <common/gl/shader_program.h>
#include <common/gl/frame_buffer_object.h>
#include <common/concurrency/executor.h>

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
		
image_transform& image_transform::operator*=(const image_transform &other)
{
	alpha *= other.alpha;
	mode = other.mode;
	uv.get<0>() += other.uv.get<0>();
	uv.get<1>() += other.uv.get<1>();
	uv.get<2>() += other.uv.get<2>();
	uv.get<3>() += other.uv.get<3>();
	return *this;
}

const image_transform image_transform::operator*(const image_transform &other) const
{
	return image_transform(*this) *= other;
}

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
	

struct renderer
{	
	renderer(const video_format_desc& format_desc) : fbo_(format_desc.width, format_desc.height), reading_(create_reading())
	{
		transform_stack_.push(image_transform());
		transform_stack_.top().mode = video_mode::progressive;
		transform_stack_.top().uv = boost::make_tuple(0.0, 1.0, 1.0, 0.0);

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
		glPushMatrix();
		transform_stack_.push(transform_stack_.top()*transform);

		glColor4d(1.0, 1.0, 1.0, transform_stack_.top().alpha);
		glTranslated(transform.pos.get<0>()*2.0, transform.pos.get<1>()*2.0, 0.0);
		
		set_mode(transform_stack_.top().mode);
	}
		
	void render(const pixel_format_desc& desc, std::vector<safe_ptr<write_buffer>>& buffers)
	{
		shaders_[desc.pix_fmt].use();

		for(size_t n = 0; n < buffers.size(); ++n)
		{
			auto buffer = buffers[n];

			glActiveTexture(GL_TEXTURE0+n);
			buffer->unmap();
						
			bool fit = buffer->width() == fbo_.width() && buffer->height() == fbo_.height();
			GL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, fit ? GL_NEAREST : GL_LINEAR));
			GL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, fit ? GL_NEAREST : GL_LINEAR));
		}

		auto t = transform_stack_.top();
		glBegin(GL_QUADS);
			glTexCoord2d(t.uv.get<0>(), t.uv.get<3>()); glVertex2d(-1.0, -1.0);
			glTexCoord2d(t.uv.get<2>(), t.uv.get<3>()); glVertex2d( 1.0, -1.0);
			glTexCoord2d(t.uv.get<2>(), t.uv.get<1>()); glVertex2d( 1.0,  1.0);
			glTexCoord2d(t.uv.get<0>(), t.uv.get<1>()); glVertex2d(-1.0,  1.0);
		glEnd();
	}

	void end()
	{
		transform_stack_.pop();
		set_mode(transform_stack_.top().mode);
		glColor4d(1.0, 1.0, 1.0, transform_stack_.top().alpha);
		glPopMatrix();
	}

	safe_ptr<const read_buffer> begin_pass()
	{
		reading_->map();
		GL(glClear(GL_COLOR_BUFFER_BIT));
		return reading_;
	}

	void end_pass()
	{
		reading_ = create_reading();
		reading_->unmap(); // Start transfer from frame buffer to texture. End with map in next tick.
	}
	
	void set_mode(video_mode::type mode)
	{
		if(mode == video_mode::upper)
			glPolygonStipple(upper_pattern);
		else if(mode == video_mode::lower)
			glPolygonStipple(lower_pattern);
		else
			glPolygonStipple(progressive_pattern);
	}

	safe_ptr<read_buffer> create_reading()
	{
		std::shared_ptr<read_buffer> frame;
		if(!pool_.try_pop(frame))		
			frame = std::make_shared<read_buffer>(fbo_.width(), fbo_.height());
		return safe_ptr<read_buffer>(frame.get(), [=](read_buffer*){pool_.push(frame);});
	}

	const ogl_context context_;
		 
	tbb::concurrent_bounded_queue<std::shared_ptr<read_buffer>> pool_;

	std::stack<image_transform> transform_stack_;

	std::unordered_map<pixel_format::type, gl::shader_program> shaders_;
	gl::fbo fbo_;

	safe_ptr<read_buffer> reading_;
};

struct image_processor::implementation : boost::noncopyable
{
	implementation(const video_format_desc& format_desc) : format_desc_(format_desc)
	{
		executor_.start();
	}
	
	void begin(const image_transform& transform)
	{
		executor_.begin_invoke([=]{get()->begin(transform);});
	}
		
	void render(const pixel_format_desc& desc, std::vector<safe_ptr<write_buffer>>& buffers)
	{
		executor_.begin_invoke([=]() mutable{get()->render(desc, buffers);});
	}

	void end()
	{
		executor_.begin_invoke([=]{get()->end();});
	}

	boost::unique_future<safe_ptr<const read_buffer>> begin_pass()
	{
		return executor_.begin_invoke([=]{return get()->begin_pass();});
	}

	void end_pass()
	{
		executor_.begin_invoke([=]{get()->end_pass();});
	}

	std::unique_ptr<renderer>& get()
	{
		if(!renderer_)
			renderer_.reset(new renderer(format_desc_));
		return renderer_;
	}

	std::vector<safe_ptr<write_buffer>> create_buffers(const pixel_format_desc& format)
	{
		std::vector<safe_ptr<write_buffer>> buffers;
		std::transform(format.planes.begin(), format.planes.end(), std::back_inserter(buffers), [&](const pixel_format_desc::plane& plane) -> safe_ptr<write_buffer>
		{
			size_t key = ((plane.channels << 24) & 0x0F000000) | ((plane.width << 12) & 0x00FFF000) | ((plane.height << 0) & 0x00000FFF);
			auto pool = &write_frames_[key];
			std::shared_ptr<write_buffer> buffer;
			if(!pool->try_pop(buffer))
			{
				executor_.begin_invoke([&]
				{
					buffer = std::make_shared<write_buffer>(plane.width, plane.height, plane.channels);
					buffer->map();
				}).get();	
			}

			return safe_ptr<write_buffer>(buffer.get(), [=](write_buffer*)
			{
				executor_.begin_invoke([=]
				{
					buffer->map();
					pool->push(buffer);
				});
			});
		});
		return buffers;
	}
		
	tbb::concurrent_unordered_map<size_t, tbb::concurrent_bounded_queue<std::shared_ptr<write_buffer>>> write_frames_;

	const video_format_desc format_desc_;
	std::unique_ptr<renderer> renderer_;
	executor executor_;	
};

image_processor::image_processor(const video_format_desc& format_desc) : impl_(new implementation(format_desc)){}
void image_processor::begin(const image_transform& transform) {	impl_->begin(transform);}
void image_processor::process(const pixel_format_desc& desc, std::vector<safe_ptr<write_buffer>>& buffers){	impl_->render(desc, buffers);}
void image_processor::end(){impl_->end();}
boost::unique_future<safe_ptr<const read_buffer>> image_processor::begin_pass(){	return impl_->begin_pass();}
void image_processor::end_pass(){impl_->end_pass();}
std::vector<safe_ptr<write_buffer>> image_processor::create_buffers(const pixel_format_desc& format){return impl_->create_buffers(format);}

}}