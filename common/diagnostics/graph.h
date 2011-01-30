#pragma once

#include "../memory/safe_ptr.h"

#include <string>

namespace caspar { namespace diagnostics {
	
class graph
{
	friend safe_ptr<graph> create_graph(const std::string& name);
	graph(const std::string& name);
public:
	void update(const std::string& name, float value);
	void color(const std::string& name, float r, float g, float b);
	void line(const std::string& name, float value, float r, float g, float b);
private:
	struct implementation;
	std::shared_ptr<implementation> impl_;
};

safe_ptr<graph> create_graph(const std::string& name);
	
}}