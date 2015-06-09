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

#pragma once

static std::string get_adjustement_glsl()
{
	return R"shader(
			/*
			** Contrast, saturation, brightness
			** Code of this function is from TGM's shader pack
			** http://irrlicht.sourceforge.net/phpBB2/viewtopic.php?t=21057
			*/

			vec3 ContrastSaturationBrightness(vec3 color, float brt, float sat, float con)
			{
				const float AvgLumR = 0.5;
				const float AvgLumG = 0.5;
				const float AvgLumB = 0.5;

				const vec3 LumCoeff = vec3(0.2125, 0.7154, 0.0721);

				vec3 AvgLumin = vec3(AvgLumR, AvgLumG, AvgLumB);
				vec3 brtColor = color * brt;
				vec3 intensity = vec3(dot(brtColor, LumCoeff));
				vec3 satColor = mix(intensity, brtColor, sat);
				vec3 conColor = mix(AvgLumin, satColor, con);
				return conColor;
			}

			/*
			** Gamma correction
			** Details: http://blog.mouaif.org/2009/01/22/photoshop-gamma-correction-shader/
			*/
			#define GammaCorrection(color, gamma)								pow(color, vec3(1.0 / gamma))

			/*
			** Levels control (input (+gamma), output)
			** Details: http://blog.mouaif.org/2009/01/28/levels-control-shader/
			*/

			#define LevelsControlInputRange(color, minInput, maxInput)				min(max(color - vec3(minInput), vec3(0.0)) / (vec3(maxInput) - vec3(minInput)), vec3(1.0))
			#define LevelsControlInput(color, minInput, gamma, maxInput)				GammaCorrection(LevelsControlInputRange(color, minInput, maxInput), gamma)
			#define LevelsControlOutputRange(color, minOutput, maxOutput) 			mix(vec3(minOutput), vec3(maxOutput), color)
			#define LevelsControl(color, minInput, gamma, maxInput, minOutput, maxOutput) 	LevelsControlOutputRange(LevelsControlInput(color, minInput, gamma, maxInput), minOutput, maxOutput)
	)shader";
}

static std::string get_blend_glsl()
{
	static std::string glsl = R"shader(
			/*
			** Photoshop & misc math
			** Blending modes, RGB/HSL/Contrast/Desaturate, levels control
			**
			** Romain Dura | Romz
			** Blog: http://blog.mouaif.org
			** Post: http://blog.mouaif.org/?p=94
			*/


			/*
			** Desaturation
			*/

			vec4 Desaturate(vec3 color, float Desaturation)
			{
				vec3 grayXfer = vec3(0.3, 0.59, 0.11);
				vec3 gray = vec3(dot(grayXfer, color));
				return vec4(mix(color, gray, Desaturation), 1.0);
			}


			/*
			** Hue, saturation, luminance
			*/

			vec3 RGBToHSL(vec3 color)
			{
				vec3 hsl;

				float fmin = min(min(color.r, color.g), color.b);
				float fmax = max(max(color.r, color.g), color.b);
				float delta = fmax - fmin;

				hsl.z = (fmax + fmin) / 2.0;

				if (delta == 0.0)
				{
					hsl.x = 0.0;
					hsl.y = 0.0;
				}
				else
				{
					if (hsl.z < 0.5)
						hsl.y = delta / (fmax + fmin);
					else
						hsl.y = delta / (2.0 - fmax - fmin);

					float deltaR = (((fmax - color.r) / 6.0) + (delta / 2.0)) / delta;
					float deltaG = (((fmax - color.g) / 6.0) + (delta / 2.0)) / delta;
					float deltaB = (((fmax - color.b) / 6.0) + (delta / 2.0)) / delta;

					if (color.r == fmax )
						hsl.x = deltaB - deltaG;
					else if (color.g == fmax)
						hsl.x = (1.0 / 3.0) + deltaR - deltaB;
					else if (color.b == fmax)
						hsl.x = (2.0 / 3.0) + deltaG - deltaR;

					if (hsl.x < 0.0)
						hsl.x += 1.0;
					else if (hsl.x > 1.0)
						hsl.x -= 1.0;
				}

				return hsl;
			}

			float HueToRGB(float f1, float f2, float hue)
			{
				if (hue < 0.0)
					hue += 1.0;
				else if (hue > 1.0)
					hue -= 1.0;
				float res;
				if ((6.0 * hue) < 1.0)
					res = f1 + (f2 - f1) * 6.0 * hue;
				else if ((2.0 * hue) < 1.0)
					res = f2;
				else if ((3.0 * hue) < 2.0)
					res = f1 + (f2 - f1) * ((2.0 / 3.0) - hue) * 6.0;
				else
					res = f1;
				return res;
			}

			vec3 HSLToRGB(vec3 hsl)
			{
				vec3 rgb;

				if (hsl.y == 0.0)
					rgb = vec3(hsl.z);
				else
				{
					float f2;

					if (hsl.z < 0.5)
						f2 = hsl.z * (1.0 + hsl.y);
					else
						f2 = (hsl.z + hsl.y) - (hsl.y * hsl.z);

					float f1 = 2.0 * hsl.z - f2;

					rgb.r = HueToRGB(f1, f2, hsl.x + (1.0/3.0));
					rgb.g = HueToRGB(f1, f2, hsl.x);
					rgb.b= HueToRGB(f1, f2, hsl.x - (1.0/3.0));
				}

				return rgb;
			}




			/*
			** Float blending modes
			** Adapted from here: http://www.nathanm.com/photoshop-blending-math/
			** But I modified the HardMix (wrong condition), Overlay, SoftLight, ColorDodge, ColorBurn, VividLight, PinLight (inverted layers) ones to have correct results
			*/

			#define BlendLinearDodgef 					BlendAddf
			#define BlendLinearBurnf 					BlendSubstractf
			#define BlendAddf(base, blend) 				min(base + blend, 1.0)
			#define BlendSubstractf(base, blend) 		max(base + blend - 1.0, 0.0)
			#define BlendLightenf(base, blend) 		max(blend, base)
			#define BlendDarkenf(base, blend) 			min(blend, base)
			#define BlendLinearLightf(base, blend) 	(blend < 0.5 ? BlendLinearBurnf(base, (2.0 * blend)) : BlendLinearDodgef(base, (2.0 * (blend - 0.5))))
			#define BlendScreenf(base, blend) 			(1.0 - ((1.0 - base) * (1.0 - blend)))
			#define BlendOverlayf(base, blend) 		(base < 0.5 ? (2.0 * base * blend) : (1.0 - 2.0 * (1.0 - base) * (1.0 - blend)))
			#define BlendSoftLightf(base, blend) 		((blend < 0.5) ? (2.0 * base * blend + base * base * (1.0 - 2.0 * blend)) : (sqrt(base) * (2.0 * blend - 1.0) + 2.0 * base * (1.0 - blend)))
			#define BlendColorDodgef(base, blend) 		((blend == 1.0) ? blend : min(base / (1.0 - blend), 1.0))
			#define BlendColorBurnf(base, blend) 		((blend == 0.0) ? blend : max((1.0 - ((1.0 - base) / blend)), 0.0))
			#define BlendVividLightf(base, blend)		((blend < 0.5) ? BlendColorBurnf(base, (2.0 * blend)) : BlendColorDodgef(base, (2.0 * (blend - 0.5))))
			#define BlendPinLightf(base, blend) 		((blend < 0.5) ? BlendDarkenf(base, (2.0 * blend)) : BlendLightenf(base, (2.0 *(blend - 0.5))))
			#define BlendHardMixf(base, blend) 		((BlendVividLightf(base, blend) < 0.5) ? 0.0 : 1.0)
			#define BlendReflectf(base, blend) 		((blend == 1.0) ? blend : min(base * base / (1.0 - blend), 1.0))


			/*
			** Vector3 blending modes
			*/

			#define Blend(base, blend, funcf) 			vec3(funcf(base.r, blend.r), funcf(base.g, blend.g), funcf(base.b, blend.b))

			#define BlendNormal(base, blend) 			(blend)
			#define BlendLighten						BlendLightenf
			#define BlendDarken						BlendDarkenf
			#define BlendMultiply(base, blend) 		(base * blend)
			#define BlendAverage(base, blend) 			((base + blend) / 2.0)
			#define BlendAdd(base, blend) 				min(base + blend, vec3(1.0))
			#define BlendSubstract(base, blend) 		max(base + blend - vec3(1.0), vec3(0.0))
			#define BlendDifference(base, blend) 		abs(base - blend)
			#define BlendNegation(base, blend) 		(vec3(1.0) - abs(vec3(1.0) - base - blend))
			#define BlendExclusion(base, blend) 		(base + blend - 2.0 * base * blend)
			#define BlendScreen(base, blend) 			Blend(base, blend, BlendScreenf)
			#define BlendOverlay(base, blend) 			Blend(base, blend, BlendOverlayf)
			#define BlendSoftLight(base, blend) 		Blend(base, blend, BlendSoftLightf)
			#define BlendHardLight(base, blend) 		BlendOverlay(blend, base)
			#define BlendColorDodge(base, blend) 		Blend(base, blend, BlendColorDodgef)
			#define BlendColorBurn(base, blend) 		Blend(base, blend, BlendColorBurnf)
			#define BlendLinearDodge					BlendAdd
			#define BlendLinearBurn					BlendSubstract
			#define BlendLinearLight(base, blend) 		Blend(base, blend, BlendLinearLightf)
			#define BlendVividLight(base, blend) 		Blend(base, blend, BlendVividLightf)
			#define BlendPinLight(base, blend) 		Blend(base, blend, BlendPinLightf)
			#define BlendHardMix(base, blend) 			Blend(base, blend, BlendHardMixf)
			#define BlendReflect(base, blend) 			Blend(base, blend, BlendReflectf)
			#define BlendGlow(base, blend) 			BlendReflect(blend, base)
			#define BlendPhoenix(base, blend) 			(min(base, blend) - max(base, blend) + vec3(1.0))
			#define BlendOpacity(base, blend, F, O) 	(F(base, blend) * O + blend * (1.0 - O))


			vec3 BlendHue(vec3 base, vec3 blend)
			{
				vec3 baseHSL = RGBToHSL(base);
				return HSLToRGB(vec3(RGBToHSL(blend).r, baseHSL.g, baseHSL.b));
			}

			vec3 BlendSaturation(vec3 base, vec3 blend)
			{
				vec3 baseHSL = RGBToHSL(base);
				return HSLToRGB(vec3(baseHSL.r, RGBToHSL(blend).g, baseHSL.b));
			}

			vec3 BlendColor(vec3 base, vec3 blend)
			{
				vec3 blendHSL = RGBToHSL(blend);
				return HSLToRGB(vec3(blendHSL.r, blendHSL.g, RGBToHSL(base).b));
			}

			vec3 BlendLuminosity(vec3 base, vec3 blend)
			{
				vec3 baseHSL = RGBToHSL(base);
				return HSLToRGB(vec3(baseHSL.r, baseHSL.g, RGBToHSL(blend).b));
			}

	)shader";

	return glsl;
}

static std::string get_chroma_glsl()
{
	static std::string glsl = R"shader(
		// Chroma keying
		// Author: Tim Eves <timseves@googlemail.com>
		//
		// This implements the Chroma key algorithm described in the paper:
		//      'Software Chroma Keying in an Imersive Virtual Environment'
		//      by F. van den Bergh & V. Lalioti
		// but as a pixel shader algorithm.
		//

		float       chroma_blend_w = chroma_blend.y - chroma_blend.x;
		const vec4  grey_xfer  = vec4(0.3, 0.59, 0.11, 0.0);

		float fma(float a, float b, float c) { return a*b + c; }

		// This allows us to implement the paper's alphaMap curve in software
		// rather than a largeish array
		float alpha_map(float d)
		{
		    return 1.0-smoothstep(chroma_blend.x, chroma_blend.y, d);
		}

		vec4 supress_spill(vec4 c, float d)
		{
		    float ds = smoothstep(chroma_spill, 1.0, d/chroma_blend.y);
		    float gl = dot(grey_xfer, c);
		    return mix(c, vec4(vec3(gl*gl), gl), ds);
		}

		// Key on green
		vec4 ChromaOnGreen(vec4 c)
		{
		    float d = fma(2.0, c.g, -c.r - c.b)/2.0;
		    c *= alpha_map(d);
		    return supress_spill(c, d);
		}

		//Key on blue
		vec4 ChromaOnBlue(vec4 c)
		{
		    float d = fma(2.0, c.b, -c.r - c.g)/2.0;
		    c *= alpha_map(d);
		    return supress_spill(c, d);
		}
	)shader";

	return glsl;
}
