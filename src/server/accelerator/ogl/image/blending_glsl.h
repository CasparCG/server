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

			vec3 ContrastSaturationBrightness(vec4 color, float brt, float sat, float con)
			{
				const float AvgLumR = 0.5;
				const float AvgLumG = 0.5;
				const float AvgLumB = 0.5;

				vec3 LumCoeff = is_hd
						? vec3(0.0722, 0.7152, 0.2126)
						: vec3(0.114, 0.587, 0.299);

				if (color.a > 0.0)
					color.rgb /= color.a;

				vec3 AvgLumin = vec3(AvgLumR, AvgLumG, AvgLumB);
				vec3 brtColor = color.rgb * brt;
				vec3 intensity = vec3(dot(brtColor, LumCoeff));
				vec3 satColor = mix(intensity, brtColor, sat);
				vec3 conColor = mix(AvgLumin, satColor, con);

				conColor.rgb *= color.a;

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
		vec4  grey_xfer  = is_hd
				? vec4(0.2126, 0.7152, 0.0722, 0)
				: vec4(0.299,  0.587,  0.114, 0);

		// This allows us to implement the paper's alphaMap curve in software
		// rather than a largeish array
		float alpha_map(float d)
		{
		    return 1.0 - smoothstep(1.0, chroma_softness, d);
		}

		// http://stackoverflow.com/questions/15095909/from-rgb-to-hsv-in-opengl-glsl
		vec3 rgb2hsv(vec3 c)
		{
			vec4 K = vec4(0.0, -1.0 / 3.0, 2.0 / 3.0, -1.0);
			vec4 p = mix(vec4(c.bg, K.wz), vec4(c.gb, K.xy), step(c.b, c.g));
			vec4 q = mix(vec4(p.xyw, c.r), vec4(c.r, p.yzx), step(p.x, c.r));

			float d = q.x - min(q.w, q.y);
			float e = 1.0e-10;
			return vec3(abs(q.z + (q.w - q.y) / (6.0 * d + e)), d / (q.x + e), q.x);
		}

		// From the same page
		vec3 hsv2rgb(vec3 c)
		{
			vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
			vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
			return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
		}

		float AngleDiff(float angle1, float angle2)
		{
			return 0.5 - abs(abs(angle1 - angle2) - 0.5);
		}

		float AngleDiffDirectional(float angle1, float angle2)
		{
			float diff = angle1 - angle2;

			return diff < -0.5
					? diff + 1.0
					: (diff > 0.5 ? diff - 1.0 : diff);
		}

		float Distance(float actual, float target)
		{
			return min(0.0, target - actual);
		}

		float ColorDistance(vec3 hsv)
		{
			float hueDiff					= AngleDiff(hsv.x, chroma_target_hue) * 2;
			float saturationDiff			= Distance(hsv.y, chroma_min_saturation);
			float brightnessDiff			= Distance(hsv.z, chroma_min_brightness);

			float saturationBrightnessScore	= max(brightnessDiff, saturationDiff);
			float hueScore					= hueDiff - chroma_hue_width;

			return -hueScore * saturationBrightnessScore;
		}

		vec3 supress_spill(vec3 c)
		{
			float hue		= c.x;
			float diff		= AngleDiffDirectional(hue, chroma_target_hue);
			float distance	= abs(diff) / chroma_spill_suppress;

			if (distance < 1)
			{
				c.x = diff < 0
						? chroma_target_hue - chroma_spill_suppress
						: chroma_target_hue + chroma_spill_suppress;
				c.y *= min(1.0, distance + chroma_spill_suppress_saturation);
			}

			return c;
		}

		// Key on any color
		vec4 ChromaOnCustomColor(vec4 c)
		{
			vec3 hsv		= rgb2hsv(c.rgb);
			float distance	= ColorDistance(hsv);
			float d			= distance * -2.0 + 1.0;
		    vec4 suppressed	= vec4(hsv2rgb(supress_spill(hsv)), 1.0);
			float alpha		= alpha_map(d);

			suppressed *= alpha;

			return chroma_show_mask ? vec4(suppressed.a, suppressed.a, suppressed.a, 1) : suppressed;
		}
	)shader";

    return glsl;
}
