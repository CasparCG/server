#pragma once

namespace caspar { namespace core { namespace text {

template<typename T>
struct color
{
	color() {}
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