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

#include "../../StdAfx.h"

#include "image_shader.h"

#include "../util/shader.h"
#include "../util/device.h"

#include "blending_glsl.h"

#include <common/gl/gl_check.h>
#include <common/env.h>

namespace caspar { namespace accelerator { namespace ogl {

std::weak_ptr<shader>	g_shader;
std::mutex				g_shader_mutex;
bool					g_blend_modes		= false;
bool					g_post_processing	= false;

std::string get_blend_color_func()
{
	return

		get_adjustement_glsl()

		+

		get_blend_glsl()

		+

		R"shader(
				vec3 get_blend_color(vec3 back, vec3 fore)
				{
					switch(blend_mode)
					{
					case  0: return BlendNormal(back, fore);
					case  1: return BlendLighten(back, fore);
					case  2: return BlendDarken(back, fore);
					case  3: return BlendMultiply(back, fore);
					case  4: return BlendAverage(back, fore);
					case  5: return BlendAdd(back, fore);
					case  6: return BlendSubstract(back, fore);
					case  7: return BlendDifference(back, fore);
					case  8: return BlendNegation(back, fore);
					case  9: return BlendExclusion(back, fore);
					case 10: return BlendScreen(back, fore);
					case 11: return BlendOverlay(back, fore);
				//	case 12: return BlendSoftLight(back, fore);
					case 13: return BlendHardLight(back, fore);
					case 14: return BlendColorDodge(back, fore);
					case 15: return BlendColorBurn(back, fore);
					case 16: return BlendLinearDodge(back, fore);
					case 17: return BlendLinearBurn(back, fore);
					case 18: return BlendLinearLight(back, fore);
					case 19: return BlendVividLight(back, fore);
					case 20: return BlendPinLight(back, fore);
					case 21: return BlendHardMix(back, fore);
					case 22: return BlendReflect(back, fore);
					case 23: return BlendGlow(back, fore);
					case 24: return BlendPhoenix(back, fore);
					case 25: return BlendHue(back, fore);
					case 26: return BlendSaturation(back, fore);
					case 27: return BlendColor(back, fore);
					case 28: return BlendLuminosity(back, fore);
					}
					return BlendNormal(back, fore);
				}

				vec4 blend(vec4 fore)
				{
				   vec4 back = texture2D(background, gl_TexCoord[1].st).bgra;
				   if(blend_mode != 0)
						fore.rgb = get_blend_color(back.rgb/(back.a+0.0000001), fore.rgb/(fore.a+0.0000001))*fore.a;
					switch(keyer)
					{
						case 1:  return fore + back; // additive
						default: return fore + (1.0-fore.a)*back; // linear
					}
				}
		)shader";
}

std::string get_simple_blend_color_func()
{
	return

		get_adjustement_glsl()

		+

		R"shader(
				vec4 blend(vec4 fore)
				{
					return fore;
				}
		)shader";
}

std::string get_chroma_func()
{
	return

		get_chroma_glsl()

		+

		R"shader(
				vec4 chroma_key(vec4 c)
				{
					return ChromaOnCustomColor(c.bgra).bgra;
				}
		)shader";
}

std::string get_post_process()
{
	return R"shader(
		if (post_processing)
		{
			gl_FragColor = post_process().bgra;
		}
		else
	)shader";
}

std::string get_no_post_process()
{
	return "";
}

std::string get_vertex()
{
	return R"shader(
			void main()
			{
				gl_TexCoord[0] = gl_MultiTexCoord0;
			//	gl_TexCoord[1] = gl_MultiTexCoord1;
				vec4 pos = ftransform();
				gl_TexCoord[1] = vec4(pos.xy, 0.0, 0.0);
				pos.x = pos.x*2.0 - 1.0;
				pos.y = pos.y*2.0 - 1.0;
				gl_Position    = pos;
			}
	)shader";
}

std::string get_fragment(bool blend_modes, bool post_processing)
{
	return R"shader(

			#version 130
			uniform sampler2D	background;
			uniform sampler2D	plane[4];
			uniform vec2		plane_size[4];
			uniform sampler2D	local_key;
			uniform sampler2D	layer_key;

			uniform bool		is_hd;
			uniform bool		has_local_key;
			uniform bool		has_layer_key;
			uniform int			blend_mode;
			uniform int			keyer;
			uniform int			pixel_format;
			uniform int			deinterlace;

			uniform float		opacity;
			uniform bool		levels;
			uniform float		min_input;
			uniform float		max_input;
			uniform float		gamma;
			uniform float		min_output;
			uniform float		max_output;

			uniform bool		csb;
			uniform float		brt;
			uniform float		sat;
			uniform float		con;

			uniform bool		post_processing;
			uniform bool		straighten_alpha;

			uniform bool		chroma;
			uniform bool		chroma_show_mask;
			uniform float		chroma_target_hue;
			uniform float		chroma_hue_width;
			uniform float		chroma_min_saturation;
			uniform float		chroma_min_brightness;
			uniform float		chroma_softness;
			uniform float		chroma_spill_suppress;
			uniform float		chroma_spill_suppress_saturation;
	)shader"

	+

	(blend_modes ? get_blend_color_func() : get_simple_blend_color_func())

	+

	get_chroma_func()

	+

	R"shader(

			vec4 ycbcra_to_rgba_sd(float Y, float Cb, float Cr, float A)
			{
				vec4 rgba;
				rgba.b = (1.164*(Y*255 - 16) + 1.596*(Cr*255 - 128))/255;
				rgba.g = (1.164*(Y*255 - 16) - 0.813*(Cr*255 - 128) - 0.391*(Cb*255 - 128))/255;
				rgba.r = (1.164*(Y*255 - 16) + 2.018*(Cb*255 - 128))/255;
				rgba.a = A;
				return rgba;
			}

			vec4 ycbcra_to_rgba_hd(float Y, float Cb, float Cr, float A)
			{
				vec4 rgba;
				rgba.b = (1.164*(Y*255 - 16) + 1.793*(Cr*255 - 128))/255;
				rgba.g = (1.164*(Y*255 - 16) - 0.534*(Cr*255 - 128) - 0.213*(Cb*255 - 128))/255;
				rgba.r = (1.164*(Y*255 - 16) + 2.115*(Cb*255 - 128))/255;
				rgba.a = A;
				return rgba;
			}

			vec4 ycbcra_to_rgba(float y, float cb, float cr, float a)
			{
				if(is_hd)
					return ycbcra_to_rgba_hd(y, cb, cr, a);
				else
					return ycbcra_to_rgba_sd(y, cb, cr, a);
			}

			vec4 get_sample(sampler2D sampler, vec2 coords, vec2 size)
			{
				switch(deinterlace)
				{
				case 1: // upper
					return texture2D(sampler, coords);
				case 2: // lower
					return texture2D(sampler, coords);
				default:
					return texture2D(sampler, coords);
				}
			}

			vec4 get_rgba_color()
			{
				switch(pixel_format)
				{
				case 0:		//gray
					return vec4(get_sample(plane[0], gl_TexCoord[0].st / gl_TexCoord[0].q, plane_size[0]).rrr, 1.0);
				case 1:		//bgra,
					return get_sample(plane[0], gl_TexCoord[0].st / gl_TexCoord[0].q, plane_size[0]).bgra;
				case 2:		//rgba,
					return get_sample(plane[0], gl_TexCoord[0].st / gl_TexCoord[0].q, plane_size[0]).rgba;
				case 3:		//argb,
					return get_sample(plane[0], gl_TexCoord[0].st / gl_TexCoord[0].q, plane_size[0]).argb;
				case 4:		//abgr,
					return get_sample(plane[0], gl_TexCoord[0].st / gl_TexCoord[0].q, plane_size[0]).gbar;
				case 5:		//ycbcr,
					{
						float y  = get_sample(plane[0], gl_TexCoord[0].st / gl_TexCoord[0].q, plane_size[0]).r;
						float cb = get_sample(plane[1], gl_TexCoord[0].st / gl_TexCoord[0].q, plane_size[0]).r;
						float cr = get_sample(plane[2], gl_TexCoord[0].st / gl_TexCoord[0].q, plane_size[0]).r;
						return ycbcra_to_rgba(y, cb, cr, 1.0);
					}
				case 6:		//ycbcra
					{
						float y  = get_sample(plane[0], gl_TexCoord[0].st / gl_TexCoord[0].q, plane_size[0]).r;
						float cb = get_sample(plane[1], gl_TexCoord[0].st / gl_TexCoord[0].q, plane_size[0]).r;
						float cr = get_sample(plane[2], gl_TexCoord[0].st / gl_TexCoord[0].q, plane_size[0]).r;
						float a  = get_sample(plane[3], gl_TexCoord[0].st / gl_TexCoord[0].q, plane_size[0]).r;
						return ycbcra_to_rgba(y, cb, cr, a);
					}
				case 7:		//luma
					{
						vec3 y3 = get_sample(plane[0], gl_TexCoord[0].st / gl_TexCoord[0].q, plane_size[0]).rrr;
						return vec4((y3-0.065)/0.859, 1.0);
					}
				case 8:		//bgr,
					return vec4(get_sample(plane[0], gl_TexCoord[0].st / gl_TexCoord[0].q, plane_size[0]).bgr, 1.0);
				case 9:		//rgb,
					return vec4(get_sample(plane[0], gl_TexCoord[0].st / gl_TexCoord[0].q, plane_size[0]).rgb, 1.0);
				}
				return vec4(0.0, 0.0, 0.0, 0.0);
			}

			vec4 post_process()
			{
				vec4 color = texture2D(background, gl_TexCoord[0].st).bgra;

				if (straighten_alpha)
					color.rgb /= color.a + 0.0000001;

				return color;
			}

			void main()
			{
	)shader"

	+

	(post_processing ? get_post_process() : get_no_post_process())

	+

	R"shader(
				{
					vec4 color = get_rgba_color();
					if (chroma)
						color = chroma_key(color);
					if(levels)
						color.rgb = LevelsControl(color.rgb, min_input, gamma, max_input, min_output, max_output);
					if(csb)
						color.rgb = ContrastSaturationBrightness(color, brt, sat, con);
					if(has_local_key)
						color *= texture2D(local_key, gl_TexCoord[1].st).r;
					if(has_layer_key)
						color *= texture2D(layer_key, gl_TexCoord[1].st).r;
					color *= opacity;
					color = blend(color);
					gl_FragColor = color.bgra;
				}
			}
	)shader";
}

std::shared_ptr<shader> get_image_shader(
		const spl::shared_ptr<device>& ogl,
		bool& blend_modes,
		bool blend_modes_wanted,
		bool& post_processing,
		bool straight_alpha_wanted)
{
	std::lock_guard<std::mutex> lock(g_shader_mutex);
	auto existing_shader = g_shader.lock();

	if(existing_shader)
	{
		blend_modes = g_blend_modes;
		post_processing = g_post_processing;
		return existing_shader;
	}

	// The deleter is alive until the weak pointer is destroyed, so we have
	// to weakly reference ogl, to not keep it alive until atexit
	std::weak_ptr<device> weak_ogl = ogl;

	auto deleter = [weak_ogl](shader* p)
	{
		auto ogl = weak_ogl.lock();

        if (ogl) {
            ogl->dispatch_async([=]
            {
                delete p;
            });
        }
	};

	try
	{
		g_blend_modes  = glTextureBarrierNV ? blend_modes_wanted : false;
		g_post_processing = straight_alpha_wanted;
		existing_shader.reset(new shader(get_vertex(), get_fragment(g_blend_modes, g_post_processing)), deleter);
	}
	catch(...)
	{
		CASPAR_LOG_CURRENT_EXCEPTION();
		CASPAR_LOG(warning) << "Failed to compile shader. Trying to compile without blend-modes.";

		g_blend_modes = false;
		existing_shader.reset(new shader(get_vertex(), get_fragment(g_blend_modes, g_post_processing)), deleter);
	}

	//if(!g_blend_modes)
	//{
	//	ogl.blend_func(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE);
	//	CASPAR_LOG(info) << L"[shader] Blend-modes are disabled.";
	//}


	blend_modes = g_blend_modes;
	post_processing = g_post_processing;
	g_shader = existing_shader;
	return existing_shader;
}

}}}
