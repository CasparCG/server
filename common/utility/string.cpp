#include "../stdafx.h"

namespace caspar {
	
std::wstring widen(const std::string& str)
{
	return std::wstring(str.begin(), str.end());
}

std::wstring widen(const std::wstring& str)
{
	return str;
}
	   
std::string narrow(const std::wstring& str)
{
	return std::string(str.begin(), str.end());
}
	   
std::string narrow(const std::string& str)
{
	return str ;
}

}