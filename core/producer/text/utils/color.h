#pragma once

namespace caspar { namespace core { namespace text {

template<typename T>
struct color
{
	color() {}
	explicit color(unsigned int value)
	{
		b =  (value & 0x000000ff)			/ 255.0;
		g = ((value & 0x0000ff00) >>  8)	/ 255.0;
		r = ((value & 0x00ff0000) >> 16)	/ 255.0;
		a = ((value & 0xff000000) >> 24)	/ 255.0;
	}

	color(T alpha, T red, T green, T blue) : r(red), g(green), b(blue), a(alpha) {}

	T r;
	T g;
	T b;
	T a;
};

}}}