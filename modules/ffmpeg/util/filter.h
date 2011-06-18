#pragma once

#include <common/memory/safe_ptr.h>

struct AVFrame;

namespace caspar {
		
class filter
{
public:
	filter(const std::string& filters);

	void push(const safe_ptr<AVFrame>& frame);
	bool try_pop(std::shared_ptr<AVFrame>& frame);
	size_t size() const;

private:
	struct implementation;
	safe_ptr<implementation> impl_;
};

}