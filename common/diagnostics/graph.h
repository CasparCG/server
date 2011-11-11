/*
* copyright (c) 2010 Sveriges Television AB <info@casparcg.com>
*
*  This file is part of CasparCG.
*
*    CasparCG is free software: you can redistribute it and/or modify
*    it under the terms of the GNU General Public License as published by
*    the Free Software Foundation, either version 3 of the License, or
*    (at your option) any later version.
*
*    CasparCG is distributed in the hope that it will be useful,
*    but WITHOUT ANY WARRANTY; without even the implied warranty of
*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*    GNU General Public License for more details.

*    You should have received a copy of the GNU General Public License
*    along with CasparCG.  If not, see <http://www.gnu.org/licenses/>.
*
*/
#pragma once

#include "../memory/safe_ptr.h"

#include <functional>
#include <string>
#include <vector>
#include <map>

#include <boost/range/iterator_range.hpp>
#include <boost/circular_buffer.hpp>

namespace caspar {
		
typedef std::function<std::wstring()> printer;
	
namespace diagnostics {
	
struct color
{
	float red;
	float green;
	float blue;
	float alpha;
	
	color(float r = 0.0f, float g = 0.0f, float b = 0.0f, float a = 1.0f)
		: red(r)
		, green(g)
		, blue(b)
		, alpha(a){}
};

class graph
{
	friend void register_graph(const safe_ptr<graph>& graph);
public:
	graph();
	void set_text(const std::string& value);
	void set_text(const std::wstring& value);
	void update_value(const std::string& name, double value);
	void set_value(const std::string& name, double value);
	void set_color(const std::string& name, color c);
	void add_tag(const std::string& name);
	void add_guide(const std::string& name, double value);
private:
	struct implementation;
	std::shared_ptr<implementation> impl_;
};

void register_graph(const safe_ptr<graph>& graph);
void show_graphs(bool value);

//namespace v2
//{
//	
//	struct data
//	{
//		float value;
//	};
//
//	class line
//	{
//	public:
//		line();
//		line(const std::wstring& name);
//		std::wstring print() const;
//		void update_value(float value);
//		void set_value(float value);
//		void set_tag(const std::wstring& tag);
//
//		boost::circular_buffer<data>& ticks();
//	private:
//		struct implementation;
//		std::shared_ptr<implementation> impl_;
//	};
//	
//	class graph;
//
//
//	class graph
//	{
//	public:
//		graph(const std::wstring& name);
//		graph(const printer& parent_printer);
//
//		void update_value(const std::wstring& name, float value);
//		void set_value(const std::wstring& name, float value);
//
//		void set_guide(const std::wstring& name, float value);
//		void set_color(const std::wstring& name, color c);
//
//		color get_color() const;
//		std::map<std::wstring, line>& get_lines();
//
//		std::wstring print() const;
//
//		safe_ptr<graph> clone() const;
//	private:
//		struct implementation;
//		std::shared_ptr<implementation> impl_;
//	};
//	
//	static safe_ptr<graph> create_graph(const std::wstring& name);
//	static safe_ptr<graph> create_graph(const printer& parent_printer);
//	static std::vector<safe_ptr<graph>> get_all_graphs();
//}

}}