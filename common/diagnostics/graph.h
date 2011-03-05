#pragma once

#include "../memory/safe_ptr.h"

#include "../utility/printer.h"

#include <string>
#include <vector>
#include <map>

#include <boost/range/iterator_range.hpp>
#include <boost/circular_buffer.hpp>

namespace caspar { namespace diagnostics {
	
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
	friend safe_ptr<graph> create_graph(const std::string& name);
	friend safe_ptr<graph> create_graph(const printer& parent_printer);
	graph(const std::string& name);
	graph(const printer& parent_printer);
public:
	void update_value(const std::string& name, float value);
	void set_value(const std::string& name, float value);
	void set_color(const std::string& name, color c);
	void add_tag(const std::string& name);
	void add_guide(const std::string& name, float value);
private:
	struct implementation;
	std::shared_ptr<implementation> impl_;
};

safe_ptr<graph> create_graph(const std::string& name);
safe_ptr<graph> create_graph(const printer& parent_printer);
	
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