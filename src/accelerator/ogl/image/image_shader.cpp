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
#include "image_shader.h"

#include "../util/device.h"
#include "../util/shader.h"

#include "blending_glsl.h"

#include <GL/glew.h>

#include <common/env.h>
#include <common/gl/gl_check.h>

namespace caspar { namespace accelerator { namespace ogl {

std::weak_ptr<shader> g_shader;
std::mutex            g_shader_mutex;

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
				   vec4 back = texture(background, TexCoord2.st).bgra;
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

std::string get_vertex()
{
    return R"shader(

			#version 450
            in vec4 TexCoordIn;
            in vec2 Position;

            out vec4 TexCoord;
            out vec4 TexCoord2;

			void main()
			{
				TexCoord = TexCoordIn;
                vec4 pos = vec4(Position, 0, 1);
				TexCoord2 = vec4(pos.xy, 0.0, 0.0);
				pos.x = pos.x*2.0 - 1.0;
				pos.y = pos.y*2.0 - 1.0;
				gl_Position = pos;
			}
	)shader";
}

std::string get_fragment()
{
    return R"shader(

			#version 450
            in vec4 TexCoord;
            in vec4 TexCoord2;
            out vec4 fragColor;

			uniform sampler2D	background;
			uniform sampler2D	plane[4];
			uniform sampler2D	local_key;
			uniform sampler2D	layer_key;

			uniform bool		is_hd;
			uniform bool		has_local_key;
			uniform bool		has_layer_key;
			uniform int			blend_mode;
			uniform int			keyer;
			uniform int			pixel_format;

            uniform bool        invert;
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

           get_blend_color_func()

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

			vec4 get_sample(sampler2D sampler, vec2 coords)
			{
				return texture2D(sampler, coords);
			}

			vec4 get_rgba_color()
			{
				switch(pixel_format)
				{
				case 0:		//gray
					return vec4(get_sample(plane[0], TexCoord.st / TexCoord.q).rrr, 1.0);
				case 1:		//bgra,
					return get_sample(plane[0], TexCoord.st / TexCoord.q).bgra;
				case 2:		//rgba,
					return get_sample(plane[0], TexCoord.st / TexCoord.q).rgba;
				case 3:		//argb,
					return get_sample(plane[0], TexCoord.st / TexCoord.q).argb;
				case 4:		//abgr,
					return get_sample(plane[0], TexCoord.st / TexCoord.q).gbar;
				case 5:		//ycbcr,
					{
						float y  = get_sample(plane[0], TexCoord.st / TexCoord.q).r;
						float cb = get_sample(plane[1], TexCoord.st / TexCoord.q).r;
						float cr = get_sample(plane[2], TexCoord.st / TexCoord.q).r;
						return ycbcra_to_rgba(y, cb, cr, 1.0);
					}
				case 6:		//ycbcra
					{
						float y  = get_sample(plane[0], TexCoord.st / TexCoord.q).r;
						float cb = get_sample(plane[1], TexCoord.st / TexCoord.q).r;
						float cr = get_sample(plane[2], TexCoord.st / TexCoord.q).r;
						float a  = get_sample(plane[3], TexCoord.st / TexCoord.q).r;
						return ycbcra_to_rgba(y, cb, cr, a);
					}
				case 7:		//luma
					{
						vec3 y3 = get_sample(plane[0], TexCoord.st / TexCoord.q).rrr;
						return vec4((y3-0.065)/0.859, 1.0);
					}
				case 8:		//bgr,
					return vec4(get_sample(plane[0], TexCoord.st / TexCoord.q).bgr, 1.0);
				case 9:		//rgb,
					return vec4(get_sample(plane[0], TexCoord.st / TexCoord.q).rgb, 1.0);
				}
				return vec4(0.0, 0.0, 0.0, 0.0);
			}

			void main()
			{
					vec4 color = get_rgba_color();
					if (chroma)
						color = chroma_key(color);
					if(levels)
						color.rgb = LevelsControl(color.rgb, min_input, gamma, max_input, min_output, max_output);
					if(csb)
						color.rgb = ContrastSaturationBrightness(color, brt, sat, con);
					if(has_local_key)
						color *= texture(local_key, TexCoord2.st).r;
					if(has_layer_key)
						color *= texture(layer_key, TexCoord2.st).r;
					color *= opacity;
                    if (invert)
                        color = 1.0 - color;
                    if (blend_mode >= 0)
					    color = blend(color);
					fragColor = color.bgra;
			}
	)shader";
}

std::shared_ptr<shader> get_image_shader(const spl::shared_ptr<device>& ogl)
{
    std::lock_guard<std::mutex> lock(g_shader_mutex);
    auto                        existing_shader = g_shader.lock();

    if (existing_shader) {
        return existing_shader;
    }

    // The deleter is alive until the weak pointer is destroyed, so we have
    // to weakly reference ogl, to not keep it alive until atexit
    std::weak_ptr<device> weak_ogl = ogl;

    auto deleter = [weak_ogl](shader* p) {
        auto ogl = weak_ogl.lock();

        if (ogl) {
            ogl->dispatch_async([=] { delete p; });
        }
    };

    existing_shader.reset(new shader(get_vertex(), get_fragment()), deleter);

    g_shader = existing_shader;

    return existing_shader;
}

}}} // namespace caspar::accelerator::ogl
