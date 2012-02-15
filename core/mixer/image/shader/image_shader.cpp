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

#include "../../../StdAfx.h"

#include "image_shader.h"

#include "../../gpu/shader.h"
#include "../../gpu/ogl_device.h"

#include "blending_glsl.h"

#include <common/gl/gl_check.h>
#include <common/env.h>

#include <tbb/mutex.h>

namespace caspar { namespace core {

std::shared_ptr<shader> g_shader;
tbb::mutex				g_shader_mutex;
bool					g_blend_modes = false;

std::string get_blend_color_func()
{
	return 
			
	get_adjustement_glsl()
		
	+

	get_blend_glsl()
		
	+
			
	"vec3 get_blend_color(vec3 back, vec3 fore)											\n"
	"{																					\n"
	"	switch(blend_mode)																\n"
	"	{																				\n"
	"	case  0: return BlendNormal(back, fore);										\n"
	"	case  1: return BlendLighten(back, fore);										\n"
	"	case  2: return BlendDarken(back, fore);										\n"
	"	case  3: return BlendMultiply(back, fore);										\n"
	"	case  4: return BlendAverage(back, fore);										\n"
	"	case  5: return BlendAdd(back, fore);											\n"
	"	case  6: return BlendSubstract(back, fore);										\n"
	"	case  7: return BlendDifference(back, fore);									\n"
	"	case  8: return BlendNegation(back, fore);										\n"
	"	case  9: return BlendExclusion(back, fore);										\n"
	"	case 10: return BlendScreen(back, fore);										\n"
	"	case 11: return BlendOverlay(back, fore);										\n"
	//"	case 12: return BlendSoftLight(back, fore);										\n"
	"	case 13: return BlendHardLight(back, fore);										\n"
	"	case 14: return BlendColorDodge(back, fore);									\n"
	"	case 15: return BlendColorBurn(back, fore);										\n"
	"	case 16: return BlendLinearDodge(back, fore);									\n"
	"	case 17: return BlendLinearBurn(back, fore);									\n"
	"	case 18: return BlendLinearLight(back, fore);									\n"
	"	case 19: return BlendVividLight(back, fore);									\n"
	"	case 20: return BlendPinLight(back, fore);										\n"
	"	case 21: return BlendHardMix(back, fore);										\n"
	"	case 22: return BlendReflect(back, fore);										\n"
	"	case 23: return BlendGlow(back, fore);											\n"
	"	case 24: return BlendPhoenix(back, fore);										\n"
	"	case 25: return BlendHue(back, fore);											\n"
	"	case 26: return BlendSaturation(back, fore);									\n"
	"	case 27: return BlendColor(back, fore);											\n"
	"	case 28: return BlendLuminosity(back, fore);									\n"
	"	}																				\n"
	"	return BlendNormal(back, fore);													\n"
	"}																					\n"
	"																					\n"																			  
	"vec4 blend(vec4 fore)																\n"
	"{																					\n"
	"   vec4 back = texture2D(background, gl_TexCoord[1].st).bgra;						\n"
	"   if(blend_mode != 0)																\n"
	"		fore.rgb = get_blend_color(back.rgb/(back.a+0.0000001), fore.rgb/(fore.a+0.0000001))*fore.a;\n"
	"	switch(keyer)																	\n"	
	"	{																				\n"	
	"		case 1:  return fore + back; // additive									\n"
	"		default: return fore + (1.0-fore.a)*back; // linear							\n"
	"	}																				\n"
	"}																					\n";			
}
		
std::string get_simple_blend_color_func()
{
	return 	
		
	get_adjustement_glsl()
			
	+

	"vec4 blend(vec4 fore)																\n"
	"{																					\n"
	"	return fore;																	\n"
	"}																					\n";
}

std::string get_vertex()
{
	return 

	"void main()																		\n"
	"{																					\n"
	"	gl_TexCoord[0] = gl_MultiTexCoord0;												\n"
	"	gl_TexCoord[1] = gl_MultiTexCoord1;												\n"
	"	gl_Position    = ftransform();													\n"
	"}																					\n";
}

std::string get_fragment(bool blend_modes)
{
	return

	"#version 130																		\n"
	"uniform sampler2D	background;														\n"
	"uniform sampler2D	plane[4];														\n"
	"uniform sampler2D	local_key;														\n"
	"uniform sampler2D	layer_key;														\n"
	"																					\n"
	"uniform bool		is_hd;															\n"
	"uniform bool		has_local_key;													\n"
	"uniform bool		has_layer_key;													\n"
	"uniform int		blend_mode;														\n"
	"uniform int		keyer;															\n"
	"uniform int		pixel_format;													\n"
	"																					\n"
	"uniform float		opacity;														\n"
	"uniform bool		levels;															\n"
	"uniform float		min_input;														\n"
	"uniform float		max_input;														\n"
	"uniform float		gamma;															\n"
	"uniform float		min_output;														\n"
	"uniform float		max_output;														\n"
	"																					\n"
	"uniform bool		csb;															\n"
	"uniform float		brt;															\n"
	"uniform float		sat;															\n"
	"uniform float		con;															\n"
	"																					\n"	

	+
		
	(blend_modes ? get_blend_color_func() : get_simple_blend_color_func())

	+
	
	"																					\n"
	"vec4 ycbcra_to_rgba_sd(float Y, float Cb, float Cr, float A)						\n"
	"{																					\n"
	"   vec4 rgba;																		\n"
	"   rgba.b = (1.164*(Y*255 - 16) + 1.596*(Cr*255 - 128))/255;						\n"
	"   rgba.g = (1.164*(Y*255 - 16) - 0.813*(Cr*255 - 128) - 0.391*(Cb*255 - 128))/255;\n"
	"   rgba.r = (1.164*(Y*255 - 16) + 2.018*(Cb*255 - 128))/255;						\n"
	"   rgba.a = A;																		\n"
	"	return rgba;																	\n"	
	"}																					\n"			
	"																					\n"
	"vec4 ycbcra_to_rgba_hd(float Y, float Cb, float Cr, float A)						\n"
	"{																					\n"
	"   vec4 rgba;																		\n"
	"   rgba.b = (1.164*(Y*255 - 16) + 1.793*(Cr*255 - 128))/255;						\n"
	"   rgba.g = (1.164*(Y*255 - 16) - 0.534*(Cr*255 - 128) - 0.213*(Cb*255 - 128))/255;\n"
	"   rgba.r = (1.164*(Y*255 - 16) + 2.115*(Cb*255 - 128))/255;						\n"
	"   rgba.a = A;																		\n"
	"	return rgba;																	\n"
	"}																					\n"			
	"																					\n"		
	"vec4 ycbcra_to_rgba(float y, float cb, float cr, float a)							\n"
	"{																					\n"
	"	if(is_hd)																		\n"
	"		return ycbcra_to_rgba_hd(y, cb, cr, a);										\n"
	"	else																			\n"
	"		return ycbcra_to_rgba_sd(y, cb, cr, a);										\n"
	"}																					\n"
	"																					\n"
	"vec4 get_rgba_color()																\n"
	"{																					\n"
	"	switch(pixel_format)															\n"
	"	{																				\n"
	"	case 0:		//gray																\n"
	"		return vec4(texture2D(plane[0], gl_TexCoord[0].st).rrr, 1.0);				\n"
	"	case 1:		//bgra,																\n"
	"		return texture2D(plane[0], gl_TexCoord[0].st).bgra;							\n"
	"	case 2:		//rgba,																\n"
	"		return texture2D(plane[0], gl_TexCoord[0].st).rgba;							\n"
	"	case 3:		//argb,																\n"
	"		return texture2D(plane[0], gl_TexCoord[0].st).argb;							\n"
	"	case 4:		//abgr,																\n"
	"		return texture2D(plane[0], gl_TexCoord[0].st).gbar;							\n"
	"	case 5:		//ycbcr,															\n"
	"		{																			\n"
	"			float y  = texture2D(plane[0], gl_TexCoord[0].st).r;					\n"
	"			float cb = texture2D(plane[1], gl_TexCoord[0].st).r;					\n"
	"			float cr = texture2D(plane[2], gl_TexCoord[0].st).r;					\n"
	"			return ycbcra_to_rgba(y, cb, cr, 1.0);									\n"
	"		}																			\n"
	"	case 6:		//ycbcra															\n"
	"		{																			\n"
	"			float y  = texture2D(plane[0], gl_TexCoord[0].st).r;					\n"
	"			float cb = texture2D(plane[1], gl_TexCoord[0].st).r;					\n"
	"			float cr = texture2D(plane[2], gl_TexCoord[0].st).r;					\n"
	"			float a  = texture2D(plane[3], gl_TexCoord[0].st).r;					\n"
	"			return ycbcra_to_rgba(y, cb, cr, a);									\n"
	"		}																			\n"
	"	case 7:		//luma																\n"
	"		{																			\n"
	"			vec3 y3 = texture2D(plane[0], gl_TexCoord[0].st).rrr;					\n"
	"			return vec4((y3-0.065)/0.859, 1.0);										\n"
	"		}																			\n"
	"	}																				\n"
	"	return vec4(0.0, 0.0, 0.0, 0.0);												\n"
	"}																					\n"
	"																					\n"
	"void main()																		\n"
	"{																					\n"
	"	vec4 color = get_rgba_color();													\n"
	"   if(levels)																		\n"
	"		color.rgb = LevelsControl(color.rgb, min_input, max_input, gamma, min_output, max_output); \n"
	"	if(csb)																			\n"
	"		color.rgb = ContrastSaturationBrightness(color.rgb, brt, sat, con);			\n"
	"	if(has_local_key)																\n"
	"		color *= texture2D(local_key, gl_TexCoord[1].st).r;							\n"
	"	if(has_layer_key)																\n"
	"		color *= texture2D(layer_key, gl_TexCoord[1].st).r;							\n"
	"	color *= opacity;																\n"
	"	color = blend(color);															\n"
	"	gl_FragColor = color.bgra;														\n"
	"}																					\n";
}

safe_ptr<shader> get_image_shader(ogl_device& ogl, bool& blend_modes)
{
	tbb::mutex::scoped_lock lock(g_shader_mutex);

	if(g_shader)
	{
		blend_modes = g_blend_modes;
		return make_safe_ptr(g_shader);
	}
		
	try
	{				
		g_blend_modes  = glTextureBarrierNV ? env::properties().get(L"configuration.blend-modes", false) : false;
		g_shader.reset(new shader(get_vertex(), get_fragment(g_blend_modes)));
	}
	catch(...)
	{
		CASPAR_LOG_CURRENT_EXCEPTION();
		CASPAR_LOG(warning) << "Failed to compile shader. Trying to compile without blend-modes.";
				
		g_blend_modes = false;
		g_shader.reset(new shader(get_vertex(), get_fragment(g_blend_modes)));
	}
						
	ogl.enable(GL_TEXTURE_2D);

	if(!g_blend_modes)
	{
		ogl.enable(GL_BLEND);
		glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE);
		CASPAR_LOG(info) << L"[shader] Blend-modes are disabled.";
	}

	blend_modes = g_blend_modes;
	return make_safe_ptr(g_shader);
}

}}
