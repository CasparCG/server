/*
* Copyright (c) 2011 Sveriges Television AB <info@casparcg.com>
*
* This file is part of CasparCG (www.casparcg.com).
*
* CasparCG is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* CasparCG is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with CasparCG. If not, see <http://www.gnu.org/licenses/>.
*
* Author: Robert Nagy, ronag89@gmail.com
*/

#pragma once

#include "../memory/safe_ptr.h"

#include <string>
#include <tuple>

namespace caspar { namespace diagnostics {
	
int color(float r, float g, float b, float a = 1.0f);
std::tuple<float, float, float, float> color(int code);

class graph
{
	friend void register_graph(const safe_ptr<graph>& graph);
public:
	graph();
	void set_text(const std::wstring& value);
	void set_value(const std::string& name, double value);
	void set_color(const std::string& name, int color);
	void set_tag(const std::string& name);
private:
	struct impl;
	std::shared_ptr<impl> impl_;
};

void register_graph(const safe_ptr<graph>& graph);
void show_graphs(bool value);

}}