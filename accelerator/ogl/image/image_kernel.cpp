/*
* Copyright (c) 2011 Sveriges Television AB <info@casparcg.com>
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

#include "image_shader.h"
#include "blending_glsl.h"

#include "../util/shader.h"
#include "../util/texture.h"
#include "../util/device.h"

#include <common/except.h>
#include <common/gl/gl_check.h>
#include <common/env.h>

#include <core/video_format.h>
#include <core/frame/pixel_format.h>
#include <core/frame/frame_transform.h>

#include <boost/lexical_cast.hpp>

namespace caspar { namespace accelerator { namespace ogl {
	
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

struct image_kernel::impl
{	
	spl::shared_ptr<device>	ogl_;
	spl::shared_ptr<shader>	shader_;
	bool					blend_modes_;
							
	impl(const spl::shared_ptr<device>& ogl)
		: ogl_(ogl)
		, shader_(ogl_->invoke([&]{return get_image_shader(blend_modes_);}))
	{
	}

	void draw(draw_params params)
	{
		static const double epsilon = 0.001;		
		
		CASPAR_ASSERT(params.pix_desc.planes.size() == params.textures.size());

		if(params.textures.empty() || !params.background)
			return;

		if(params.transform.opacity < epsilon)
			return;
				
		// Bind textures

		for(int n = 0; n < params.textures.size(); ++n)
			params.textures[n]->bind(n);

		if(params.local_key)
			params.local_key->bind(texture_id::local_key);
		
		if(params.layer_key)
			params.layer_key->bind(texture_id::layer_key);
			
		// Setup shader
								
		shader_->use();

		shader_->set("plane[0]",		texture_id::plane0);
		shader_->set("plane[1]",		texture_id::plane1);
		shader_->set("plane[2]",		texture_id::plane2);
		shader_->set("plane[3]",		texture_id::plane3);
		for(int n = 0; n < params.textures.size(); ++n)
			shader_->set("plane_size[" + boost::lexical_cast<std::string>(n) + "]",	
						 static_cast<float>(params.textures[n]->width()), 
						 static_cast<float>(params.textures[n]->height()));

		shader_->set("local_key",		texture_id::local_key);
		shader_->set("layer_key",		texture_id::layer_key);
		shader_->set("is_hd",		 	params.pix_desc.planes.at(0).height > 700 ? 1 : 0);
		shader_->set("has_local_key",	params.local_key);
		shader_->set("has_layer_key",	params.layer_key);
		shader_->set("pixel_format",	params.pix_desc.format.value());	
		shader_->set("opacity",			params.transform.is_key ? 1.0 : params.transform.opacity);	
				

		// Setup blend_func
		
		if(params.transform.is_key)
			params.blend_mode = core::blend_mode::normal;

		if(blend_modes_)
		{
			params.background->bind(texture_id::background);

			shader_->set("background",	texture_id::background);
			shader_->set("blend_mode",	params.blend_mode.value());
			shader_->set("keyer",		params.keyer.value());
		}
		else
		{
			GL(glEnable(GL_BLEND));

			switch(params.keyer.value())
			{
			case keyer::additive:
				GL(glBlendFunc(GL_ONE, GL_ONE));	
				break;
			case keyer::linear:
			default:			
				GL(glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA));
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
		
		if(params.transform.field_mode != core::field_mode::progressive)	
		{
			GL(glEnable(GL_POLYGON_STIPPLE));

			if(params.transform.field_mode == core::field_mode::upper)
				glPolygonStipple(upper_pattern);
			else if(params.transform.field_mode == core::field_mode::lower)
				glPolygonStipple(lower_pattern);
		}

		// Setup drawing area
		
		GL(glViewport(0, 0, params.background->width(), params.background->height()));
										
		auto m_p = params.transform.clip_translation;
		auto m_s = params.transform.clip_scale;

		bool scissor = m_p[0] > std::numeric_limits<double>::epsilon()			|| m_p[1] > std::numeric_limits<double>::epsilon() ||
					   m_s[0] < (1.0 - std::numeric_limits<double>::epsilon())	|| m_s[1] < (1.0 - std::numeric_limits<double>::epsilon());

		if(scissor)
		{
			double w = static_cast<double>(params.background->width());
			double h = static_cast<double>(params.background->height());
		
			GL(glEnable(GL_SCISSOR_TEST));
			glScissor(static_cast<int>(m_p[0]*w), static_cast<int>(m_p[1]*h), static_cast<int>(m_s[0]*w), static_cast<int>(m_s[1]*h));
		}

		auto f_p = params.transform.fill_translation;
		auto f_s = params.transform.fill_scale;
		
		// Synchronize and set render target
								
		if(blend_modes_)
		{
			// http://www.opengl.org/registry/specs/NV/texture_barrier.txt
			// This allows us to use framebuffer (background) both as source and target while blending.
			glTextureBarrierNV(); 
		}

		params.background->attach();
		
		// Draw
				
		glBegin(GL_QUADS);
			glMultiTexCoord2d(GL_TEXTURE0, 0.0, 0.0); glMultiTexCoord2d(GL_TEXTURE1,  f_p[0]        ,  f_p[1]        );		glVertex2d( f_p[0]        *2.0-1.0,  f_p[1]        *2.0-1.0);
			glMultiTexCoord2d(GL_TEXTURE0, 1.0, 0.0); glMultiTexCoord2d(GL_TEXTURE1, (f_p[0]+f_s[0]),  f_p[1]        );		glVertex2d((f_p[0]+f_s[0])*2.0-1.0,  f_p[1]        *2.0-1.0);
			glMultiTexCoord2d(GL_TEXTURE0, 1.0, 1.0); glMultiTexCoord2d(GL_TEXTURE1, (f_p[0]+f_s[0]), (f_p[1]+f_s[1]));		glVertex2d((f_p[0]+f_s[0])*2.0-1.0, (f_p[1]+f_s[1])*2.0-1.0);
			glMultiTexCoord2d(GL_TEXTURE0, 0.0, 1.0); glMultiTexCoord2d(GL_TEXTURE1,  f_p[0]        , (f_p[1]+f_s[1]));		glVertex2d( f_p[0]        *2.0-1.0, (f_p[1]+f_s[1])*2.0-1.0);
		glEnd();
		
		// Cleanup
		
		GL(glDisable(GL_SCISSOR_TEST));
		GL(glDisable(GL_POLYGON_STIPPLE));
		GL(glDisable(GL_BLEND));
	}
};

image_kernel::image_kernel(const spl::shared_ptr<device>& ogl) : impl_(new impl(ogl)){}
image_kernel::~image_kernel(){}
void image_kernel::draw(const draw_params& params){impl_->draw(params);}
bool image_kernel::has_blend_modes() const{return impl_->blend_modes_;}

}}}