/*
* Copyright 2013 Sveriges Television AB http://casparcg.com/
*
* This file is part of CasparCG (www.casparcg.com).
*
* CasparCG is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* CasparCG is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with CasparCG. If not, see <http://www.gnu.org/licenses/>.
*
* Author: Robert Nagy, ronag89@gmail.com
*/

#include "../../stdafx.h"

#include "image_kernel.h"

#include "shader/image_shader.h"
#include "shader/blending_glsl.h"

#include "../gpu/shader.h"
#include "../gpu/device_buffer.h"
#include "../gpu/ogl_device.h"

#include <common/exception/exceptions.h>
#include <common/gl/gl_check.h>
#include <common/env.h>

#include <core/video_format.h>
#include <core/producer/frame/pixel_format.h>
#include <core/producer/frame/frame_transform.h>

#include <boost/noncopyable.hpp>

namespace caspar { namespace core {

// http://stackoverflow.com/questions/563198/how-do-you-detect-where-two-line-segments-intersect
bool get_line_intersection(
		double p0_x, double p0_y,
		double p1_x, double p1_y, 
		double p2_x, double p2_y,
		double p3_x, double p3_y,
		double& result_x, double& result_y)
{
	double s1_x = p1_x - p0_x;
	double s1_y = p1_y - p0_y;
	double s2_x = p3_x - p2_x;
	double s2_y = p3_y - p2_y;

	double s = (-s1_y * (p0_x - p2_x) + s1_x * (p0_y - p2_y)) / (-s2_x * s1_y + s1_x * s2_y);
	double t = ( s2_x * (p0_y - p2_y) - s2_y * (p0_x - p2_x)) / (-s2_x * s1_y + s1_x * s2_y);

	if (s >= 0 && s <= 1 && t >= 0 && t <= 1)
	{
		// Collision detected
		result_x = p0_x + (t * s1_x);
		result_y = p0_y + (t * s1_y);

		return true;
	}

	return false; // No collision
}

double hypotenuse(double x1, double y1, double x2, double y2)
{
	auto x = x2 - x1;
	auto y = y2 - y1;

	return std::sqrt(x * x + y * y);
}

double calc_q(double close_diagonal, double distant_diagonal)
{
	return (close_diagonal + distant_diagonal) / distant_diagonal;
}

bool is_above_screen(double y)
{
	return y < 0.0;
}

bool is_below_screen(double y)
{
	return y > 1.0;
}

bool is_left_of_screen(double x)
{
	return x < 0.0;
}

bool is_right_of_screen(double x)
{
	return x > 1.0;
}

bool is_outside_screen(
		double x1, double y1,
		double x2, double y2,
		double x3, double y3,
		double x4, double y4)
{
	// Every point needs to be outside the screen on the *same* side in order to be considered outside the screen.
	return (is_above_screen(y1) && is_above_screen(y2) && is_above_screen(y3) && is_above_screen(y4))
		|| (is_below_screen(y1) && is_below_screen(y2) && is_below_screen(y3) && is_below_screen(y4))
		|| (is_left_of_screen(x1) && is_left_of_screen(x2) && is_left_of_screen(x3) && is_left_of_screen(x4))
		|| (is_right_of_screen(x1) && is_right_of_screen(x2) && is_right_of_screen(x3) && is_right_of_screen(x4));
}

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
	safe_ptr<ogl_device>	ogl_;
	safe_ptr<shader>		shader_;
	bool					blend_modes_;
	bool					post_processing_;
	bool					supports_texture_barrier_;
							
	implementation(const safe_ptr<ogl_device>& ogl)
		: ogl_(ogl)
		, shader_(ogl_->invoke([&]{return get_image_shader(*ogl, blend_modes_, post_processing_);}))
		, supports_texture_barrier_(glTextureBarrierNV != 0)
	{
		if (!supports_texture_barrier_)
			CASPAR_LOG(warning) << L"[image_mixer] TextureBarrierNV not supported. Post processing will not be available";
	}

	void draw(draw_params&& params)
	{
		static const double epsilon = 0.001;

		CASPAR_ASSERT(params.pix_desc.planes.size() == params.textures.size());

		if(params.textures.empty() || !params.background)
			return;

		if(params.transform.opacity < epsilon)
			return;

		auto f_p = params.transform.fill_translation;
		auto f_s = params.transform.fill_scale;

		// Calculate rotation
		auto aspect = params.aspect_ratio;
		auto angle = params.transform.angle;

		auto rotate = [angle, aspect](double orig_x, double orig_y) -> boost::array<double, 2>
		{
			boost::array<double, 2> result;
			result[0] = orig_x * std::cos(angle) - orig_y * std::sin(angle);
			result[1] = orig_x * std::sin(angle) + orig_y * std::cos(angle);
			result[1] *= aspect;

			return result;
		};

		auto anchor = params.transform.anchor;
		auto crop = params.transform.crop;
		auto pers = params.transform.perspective;

		auto ul = rotate((-anchor[0] + pers.ul[0] + crop.ul[0]      ) * f_s[0], (-anchor[1] + pers.ul[1] + crop.ul[1]      ) * f_s[1] / aspect);
		auto ur = rotate((-anchor[0] + pers.ur[0] + crop.lr[0] - 1.0) * f_s[0], (-anchor[1] + pers.ur[1] + crop.ul[1]      ) * f_s[1] / aspect);
		auto lr = rotate((-anchor[0] + pers.lr[0] + crop.lr[0] - 1.0) * f_s[0], (-anchor[1] + pers.lr[1] + crop.lr[1] - 1.0) * f_s[1] / aspect);
		auto ll = rotate((-anchor[0] + pers.ll[0] + crop.ul[0]      ) * f_s[0], (-anchor[1] + pers.ll[1] + crop.lr[1] - 1.0) * f_s[1] / aspect);

		auto upper_left_x =  f_p[0] + ul[0];
		auto upper_left_y =  f_p[1] + ul[1];
		auto upper_right_x = f_p[0] + ur[0];
		auto upper_right_y = f_p[1] + ur[1];
		auto lower_right_x = f_p[0] + lr[0];
		auto lower_right_y = f_p[1] + lr[1];
		auto lower_left_x =  f_p[0] + ll[0];
		auto lower_left_y =  f_p[1] + ll[1];

		// Skip drawing if the QUAD will be outside the screen.
		if (is_outside_screen(
					upper_left_x, upper_left_y,
					upper_right_x, upper_right_y,
					lower_right_x, lower_right_y,
					lower_left_x, lower_left_y))
		{
			return;
		}
		
		if(!std::all_of(params.textures.begin(), params.textures.end(), std::mem_fn(&device_buffer::ready)))
		{
			CASPAR_LOG(trace) << L"[image_mixer] Performance warning. Host to device transfer not complete, GPU will be stalled";
			ogl_->yield(); // Try to give it some more time.
		}		
		
		// Bind textures

		for(size_t n = 0; n < params.textures.size(); ++n)
			params.textures[n]->bind(n);

		if(params.local_key)
			params.local_key->bind(texture_id::local_key);
		
		if(params.layer_key)
			params.layer_key->bind(texture_id::layer_key);
			
		// Setup shader
								
		ogl_->use(*shader_);

		shader_->set("plane[0]",		texture_id::plane0);
		shader_->set("plane[1]",		texture_id::plane1);
		shader_->set("plane[2]",		texture_id::plane2);
		shader_->set("plane[3]",		texture_id::plane3);
		shader_->set("local_key",		texture_id::local_key);
		shader_->set("layer_key",		texture_id::layer_key);
		shader_->set("is_hd",		 	params.pix_desc.planes.at(0).height > 700 ? 1 : 0);
		shader_->set("has_local_key",	bool(params.local_key));
		shader_->set("has_layer_key",	bool(params.layer_key));
		shader_->set("pixel_format",	params.pix_desc.pix_fmt);	
		shader_->set("opacity",			params.transform.is_key ? 1.0 : params.transform.opacity);	
		shader_->set("post_processing",	false);

		shader_->set("chroma_mode",    params.blend_mode.chroma.key == chroma::green ? 1 : (params.blend_mode.chroma.key == chroma::blue ? 2 : 0));
        shader_->set("chroma_blend",   params.blend_mode.chroma.threshold, params.blend_mode.chroma.softness);
        shader_->set("chroma_spill",   params.blend_mode.chroma.spill);
//        shader_->set("chroma.key",      ((params.blend_mode.chroma.key >> 24) && 0xff)/255.0f,
//                                        ((params.blend_mode.chroma.key >> 16) && 0xff)/255.0f,
//                                        (params.blend_mode.chroma.key & 0xff)/255.0f);
//		if (params.blend_mode.chroma.key != chroma::none)
//		{
//		    shader_->set("chroma.threshold", 	params.blend_mode.chroma.threshold);
//		    shader_->set("chroma.softness",     params.blend_mode.chroma.softness);
//            shader_->set("chroma.blur",         params.blend_mode.chroma.blur);
//		    shader_->set("chroma.spill",        params.blend_mode.chroma.spill);
//            shader_->set("chroma.show_mask",    params.blend_mode.chroma.show_mask);
//		}
		
		// Setup blend_func		
		if(params.transform.is_key)
			params.blend_mode = blend_mode::normal;

		if(blend_modes_)
		{
			params.background->bind(texture_id::background);

			shader_->set("background",	texture_id::background);
			shader_->set("blend_mode",	params.blend_mode.mode);
			shader_->set("keyer",		params.keyer);
		}
		else
		{
			switch(params.keyer)
			{
			case keyer::additive:
				ogl_->blend_func(GL_ONE, GL_ONE);	
				break;
			case keyer::linear:
			default:				
				ogl_->blend_func(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);	
			}		
		}

		// Setup image-adjustements
		
		if(params.transform.levels.min_input  > epsilon		||
		   params.transform.levels.max_input  < 1.0-epsilon	||
		   params.transform.levels.min_output > epsilon		||
		   params.transform.levels.max_output < 1.0-epsilon	||
		   std::abs(params.transform.levels.gamma - 1.0) > epsilon)
		{
			shader_->set("levels", true);	
			shader_->set("min_input",	params.transform.levels.min_input);	
			shader_->set("max_input",	params.transform.levels.max_input);
			shader_->set("min_output",	params.transform.levels.min_output);
			shader_->set("max_output",	params.transform.levels.max_output);
			shader_->set("gamma",		params.transform.levels.gamma);
		}
		else
			shader_->set("levels", false);	

		if(std::abs(params.transform.brightness - 1.0) > epsilon ||
		   std::abs(params.transform.saturation - 1.0) > epsilon ||
		   std::abs(params.transform.contrast - 1.0)   > epsilon)
		{
			shader_->set("csb",	true);	
			
			shader_->set("brt", params.transform.brightness);	
			shader_->set("sat", params.transform.saturation);
			shader_->set("con", params.transform.contrast);
		}
		else
			shader_->set("csb",	false);	
		
		// Setup interlacing

		if(params.transform.field_mode == core::field_mode::progressive)			
			ogl_->disable(GL_POLYGON_STIPPLE);			
		else			
		{
			ogl_->enable(GL_POLYGON_STIPPLE);

			if(params.transform.field_mode == core::field_mode::upper)
				ogl_->stipple_pattern(upper_pattern);
			else if(params.transform.field_mode == core::field_mode::lower)
				ogl_->stipple_pattern(lower_pattern);
		}

		// Setup drawing area
		
		ogl_->viewport(0, (params.transform.is_paused && params.transform.field_mode == core::field_mode::upper) ? 1 : 0, params.background->width(), params.background->height());
								
		auto m_p = params.transform.clip_translation;
		auto m_s = params.transform.clip_scale;

		bool scissor = m_p[0] > std::numeric_limits<double>::epsilon()			|| m_p[1] > std::numeric_limits<double>::epsilon() ||
					   m_s[0] < (1.0 - std::numeric_limits<double>::epsilon())	|| m_s[1] < (1.0 - std::numeric_limits<double>::epsilon());

		if(scissor)
		{
			double w = static_cast<double>(params.background->width());
			double h = static_cast<double>(params.background->height());
		
			ogl_->enable(GL_SCISSOR_TEST);
			ogl_->scissor(static_cast<size_t>(m_p[0]*w), static_cast<size_t>(m_p[1]*h), static_cast<size_t>(m_s[0]*w), static_cast<size_t>(m_s[1]*h));
		}

		// Set render target
		
		ogl_->attach(*params.background);
		
		// Perspective correction
		auto ulq = 1.0;
		auto urq = 1.0;
		auto lrq = 1.0;
		auto llq = 1.0;
		double diagonal_intersection_x;
		double diagonal_intersection_y;

		if (get_line_intersection(
				pers.ul[0] + crop.ul[0]      , pers.ul[1] + crop.ul[1]      ,
				pers.lr[0] + crop.lr[0] - 1.0, pers.lr[1] + crop.lr[1] - 1.0,
				pers.ur[0] + crop.lr[0] - 1.0, pers.ur[1] + crop.ul[1]      ,
				pers.ll[0] + crop.ul[0]      , pers.ll[1] + crop.lr[1] - 1.0,
				diagonal_intersection_x,
				diagonal_intersection_y))
		{
			// http://www.reedbeta.com/blog/2012/05/26/quadrilateral-interpolation-part-1/
			auto d0 = hypotenuse(pers.ll[0] + crop.ul[0]      , pers.ll[1] + crop.lr[1] - 1.0, diagonal_intersection_x, diagonal_intersection_y);
			auto d1 = hypotenuse(pers.lr[0] + crop.lr[0] - 1.0, pers.lr[1] + crop.lr[1] - 1.0, diagonal_intersection_x, diagonal_intersection_y);
			auto d2 = hypotenuse(pers.ur[0] + crop.lr[0] - 1.0, pers.ur[1] + crop.ul[1]      , diagonal_intersection_x, diagonal_intersection_y);
			auto d3 = hypotenuse(pers.ul[0] + crop.ul[0]      , pers.ul[1] + crop.ul[1]      , diagonal_intersection_x, diagonal_intersection_y);

			ulq = calc_q(d3, d1);
			urq = calc_q(d2, d0);
			lrq = calc_q(d1, d3);
			llq = calc_q(d0, d2);
		}

		// Draw
		/*
			GL_TEXTURE0 are texture coordinates to the source material, what will be rendered with this call. These are always set to the whole thing.
			GL_TEXTURE1 are texture coordinates to background- / key-material, that which will have to be taken in consideration when blending. These are set to the rectangle over which the source will be rendered
		*/
		glBegin(GL_QUADS);
			glMultiTexCoord4d(GL_TEXTURE0, crop.ul[0] * ulq, crop.ul[1] * ulq, 0, ulq); glMultiTexCoord2d(GL_TEXTURE1, upper_left_x,  upper_left_y);		glVertex2d(upper_left_x  * 2.0 - 1.0, upper_left_y  * 2.0 - 1.0);
			glMultiTexCoord4d(GL_TEXTURE0, crop.lr[0] * urq, crop.ul[1] * urq, 0, urq); glMultiTexCoord2d(GL_TEXTURE1, upper_right_x, upper_right_y);		glVertex2d(upper_right_x * 2.0 - 1.0, upper_right_y * 2.0 - 1.0);
			glMultiTexCoord4d(GL_TEXTURE0, crop.lr[0] * lrq, crop.lr[1] * lrq, 0, lrq); glMultiTexCoord2d(GL_TEXTURE1, lower_right_x, lower_right_y);		glVertex2d(lower_right_x * 2.0 - 1.0, lower_right_y * 2.0 - 1.0);
			glMultiTexCoord4d(GL_TEXTURE0, crop.ul[0] * llq, crop.lr[1] * llq, 0, llq); glMultiTexCoord2d(GL_TEXTURE1, lower_left_x,  lower_left_y);		glVertex2d(lower_left_x  * 2.0 - 1.0, lower_left_y  * 2.0 - 1.0);
		glEnd();
		
		// Cleanup

		ogl_->disable(GL_SCISSOR_TEST);
						
		params.textures.clear();
		ogl_->yield(); // Return resources to pool as early as possible.

		if(blend_modes_)
		{
			// http://www.opengl.org/registry/specs/NV/texture_barrier.txt
			// This allows us to use framebuffer (background) both as source and target while blending.
			glTextureBarrierNV(); 
		}
	}

	void post_process(
			const safe_ptr<device_buffer>& background, bool straighten_alpha)
	{
		bool should_post_process = 
				supports_texture_barrier_
				&& straighten_alpha
				&& post_processing_;

		if (!should_post_process)
			return;

		if (!blend_modes_)
			ogl_->disable(GL_BLEND);

		ogl_->disable(GL_POLYGON_STIPPLE);

		ogl_->attach(*background);

		background->bind(texture_id::background);

		ogl_->use(*shader_);
		shader_->set("background", texture_id::background);
		shader_->set("post_processing", should_post_process);
		shader_->set("straighten_alpha", straighten_alpha);

		ogl_->viewport(0, 0, background->width(), background->height());

		glBegin(GL_QUADS);
			glMultiTexCoord2d(GL_TEXTURE0, 0.0, 0.0); glVertex2d(-1.0, -1.0);
			glMultiTexCoord2d(GL_TEXTURE0, 1.0, 0.0); glVertex2d( 1.0, -1.0);
			glMultiTexCoord2d(GL_TEXTURE0, 1.0, 1.0); glVertex2d( 1.0,  1.0);
			glMultiTexCoord2d(GL_TEXTURE0, 0.0, 1.0); glVertex2d(-1.0,  1.0);
		glEnd();

		glTextureBarrierNV();

		if (!blend_modes_)
			ogl_->enable(GL_BLEND);
	}
};

image_kernel::image_kernel(const safe_ptr<ogl_device>& ogl) : impl_(new implementation(ogl)){}
void image_kernel::draw(draw_params&& params)
{
	impl_->draw(std::move(params));
}

void image_kernel::post_process(
		const safe_ptr<device_buffer>& background, bool straighten_alpha)
{
	impl_->post_process(background, straighten_alpha);
}

}}
