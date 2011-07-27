#pragma once

#include <common/memory/safe_ptr.h>

#include <string>
#include <vector>

struct AVFrame;

namespace caspar {
		
static bool double_rate(const std::wstring& filters)
{
	if(filters.find(L"YADIF=1") != std::string::npos)
		return true;
	
	if(filters.find(L"YADIF=3") != std::string::npos)
		return true;

	return false;
}

class filter
{
public:
	filter(const std::wstring& filters);

	void push(const safe_ptr<AVFrame>& frame);
	std::vector<safe_ptr<AVFrame>> poll();

private:
	struct implementation;
	safe_ptr<implementation> impl_;
};

}