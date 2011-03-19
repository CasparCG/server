#pragma once

#include <boost/assign/list_of.hpp>
#include <unordered_map>
#include <string>
#include <locale>

namespace caspar {
			
static const double PI = std::atan(1.0)*4.0;
static const double H_PI = std::atan(1.0)*2.0;

template<typename T>
inline T ease_none(T b, T e, T d) 
{
	return b + (e-b)*d;
}

template<typename T>
inline T ease_in_sine(T b, T e, T d)  
{
	return -(e-b) * std::cos(d * H_PI) + (e-b) + b;
}

template<typename T>
inline T ease_out_sine(T b, T e, T d)  
{ 
	return (e-b) * std::sin(d * H_PI) + b;
}

template<typename T>
inline T ease_in_out_sine(T b, T e, T d)  
{ 
	return -(e-b)/2 * (std::cos(PI*d) - 1.0) + b;
}

template<typename T>
inline T ease_out_in_sine(T b, T e, T d)  
{
	return d < 0.5 ? ease_out_sine(b, e+(e-b)*0.5, d*2.0) : ease_out_sine(b+(e-b)*0.5, e, d*2.0-1.0);
}

template<typename T>
inline T ease_in_cubic(T b, T e, T d)  
{ 
	return (e-b) * std::pow(d, 3) + b;
}

template<typename T>
inline T ease_out_cubic(T b, T e, T d)  
{ 
	return (e-b) * (std::pow(d-1.0, 3)+1.0) + b;
}
	
template<typename T>
inline T ease_in_out_cubic(T b, T e, T d)  
{ 
	return d < 0.5 ? (e-b)*0.5 * std::pow(d*2.0, 3) + b : (e-b)*0.5 * (std::pow(d*2.0-2.0, 3)+2.0)+ b;
}

template<typename T>
inline T ease_out_in_cubic(T b, T e, T d)  
{ 
	return d < 0.5 ? ease_out_cubic(b, e+(e-b)*0.5, d*2.0) : ease_in_cubic(b+(e-b)*0.5, e, d*2.0-1.0);
}

template<typename T>
inline std::function<T(T,T,T)> get_tweener(std::wstring name = L"linear")
{
	std::transform(name.begin(), name.end(), name.begin(), std::tolower);

	typedef std::function<T(T,T,T)> tween_func;
	static const std::unordered_map<std::wstring, tween_func> tweens = boost::assign::map_list_of
		(L"linear", ease_none<T>)
		(L"easenone", ease_none<T>)
		(L"easeinsine", ease_in_sine<T>)
		(L"easeoutsine", ease_out_sine<T>)
		(L"easeinoutsine", ease_in_out_sine<T>)
		(L"easeoutinsine", ease_out_in_sine<T>)
		(L"easeincubic", ease_in_cubic<T>)
		(L"easeoutcubic", ease_out_cubic<T>)
		(L"easeinoutcubic", ease_in_out_cubic<T>)
		(L"easeoutincubic", ease_out_in_cubic<T>);

	auto it = tweens.find(name);
	if(it == tweens.end())
	{
		CASPAR_LOG(warning) << L" Invalid tween: " << name << L" fallback to \"linear\".";
		it = tweens.find(L"linear");
	}

	return it->second;
};

}