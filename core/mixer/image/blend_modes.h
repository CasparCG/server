#pragma once

namespace caspar { namespace core {
		
struct blend_mode
{
	enum type 
	{
		normal = 0,
		lighten,
		darken,
		multiply,
		average,
		add,
		subtract,
		difference,
		negation,
		exclusion,
		screen,
		overlay,
		soft_light,
		hard_light,
		color_dodge,
		color_burn,
		linear_dodge,
		linear_burn,
		linear_light,
		vivid_light,
		pin_light,
		hard_mix,
		reflect,
		glow,
		phoenix,
		contrast,
		saturation,
		color,
		luminosity,
		blend_mode_count 
	};
};

blend_mode::type get_blend_mode(const std::wstring& str);

}}