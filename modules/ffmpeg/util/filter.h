#pragma once

#include <common/memory/safe_ptr.h>

#include <vector>

struct AVFrame;

namespace caspar {
		
class filter
{
public:
	filter(const std::string& filters);

	std::vector<safe_ptr<AVFrame>> execute(const safe_ptr<AVFrame>& frame);

private:
	struct implementation;
	safe_ptr<implementation> impl_;
};

}