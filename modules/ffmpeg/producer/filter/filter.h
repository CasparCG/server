#pragma once

#include <common/memory/safe_ptr.h>

#include <vector>

struct AVFrame;

namespace caspar {
		
class filter
{
public:
	filter(const std::string& filters);

	void push(const safe_ptr<AVFrame>& frame);
	std::vector<safe_ptr<AVFrame>> poll();
	bool is_ready() const;

private:
	struct implementation;
	safe_ptr<implementation> impl_;
};

}