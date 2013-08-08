#pragma once

namespace caspar { namespace core { namespace text {

template<typename T>
struct color
{
	color() {}
	color(unsigned int value)
	{
		b =  (value & 0x000000ff)			/ 255.0f;
		g = ((value & 0x0000ff00) >>  8)	/ 255.0f;
		r = ((value & 0x00ff0000) >> 16)	/ 255.0f;
		a = ((value & 0xff000000) >> 24)	/ 255.0f;
	}

	color(const color& other) : r(other.r), g(other.g), b(other.b), a(other.a) {}
	color(T alpha, T red, T green, T blue) : r(red), g(green), b(blue), a(alpha) {}

	const color&operator=(const color& other)
	{
		r = other.r;
		g = other.g;
		b = other.b;
		a = other.a;

		return *this;
	}

	T r;
	T g;
	T b;
	T a;
};

}}}