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
* Author: Niklas P Andersson, niklas.p.andersson@svt.se
*/

#include "pdf_reader.h"

#include <common/log.h>

#pragma warning(push)
#pragma warning(disable: 4459)
#pragma warning(disable: 4244)
#include <boost/spirit/home/qi.hpp>
#pragma warning(pop)

#include <boost/lexical_cast.hpp>

#include <cstdint>

namespace qi = boost::spirit::qi;

namespace caspar { namespace psd { 

struct pdf_context
{
	typedef boost::property_tree::wptree Ptree;
	typedef std::vector<char>::iterator It;

	std::wstring value;
	bool char_flag;
	Ptree root;
	std::wstring name;
	std::vector<Ptree*> stack;

	void clear_state()
	{
		name.clear();
		value.clear();
		char_flag = false;
	}

	void start_object()
	{
		if(stack.empty())
			stack.push_back(&root);
		else
		{
			Ptree* parent = stack.back();
			Ptree* child = &parent->push_back(std::make_pair(name, Ptree()))->second;
			stack.push_back(child);

			clear_state();
		}
	}

	void end_object()
	{
		BOOST_ASSERT(stack.size() >= 1);
		stack.pop_back();
	}

	std::string get_indent() const
	{
		return std::string(stack.size() * 4, ' ');
	}

	void set_name(const std::string& str)
	{
		name.assign(str.begin(), str.end());
		CASPAR_LOG(debug) << get_indent() << name;
	}

	void add_char(std::uint8_t c)
	{
		char_flag = !char_flag;

		if(char_flag)
			value.push_back(c << 8);
		else
			value[value.size()-1] = value[value.size()-1] + c;
	}

	void set_value(bool val)
	{
		value = val ? L"true" : L"false";
		set_value();
	}
	void set_value(double val)
	{
		value = boost::lexical_cast<std::wstring>(val);
		set_value();
	}

	void set_value()
	{
		CASPAR_LOG(debug) << get_indent() << value;

		stack.back()->push_back(std::make_pair(name, Ptree(value)));
		clear_state();
	}
};

template<typename Iterator>
struct pdf_grammar : qi::grammar<Iterator, qi::space_type>
{
	qi::rule<Iterator, qi::space_type> root, object, member, value, list;
	qi::rule<Iterator, qi::space_type> string;
	qi::rule<Iterator, std::string()> name_str;
	qi::rule<Iterator> escape;
	qi::rule<Iterator> name;

	mutable pdf_context context;

	pdf_grammar() : pdf_grammar::base_type(root, "pdf-grammar")
	{
		root 
			=	object >> qi::eoi;
			
		object 
			=	qi::lit("<<")[a_object_start(context)] >> *member >> qi::lit(">>")[a_object_end(context)];

		member 
			=	qi::char_('/') >> name >> value;

		name 
			=	name_str[a_name(context)];

		name_str 
			=	(qi::alpha >> *qi::alnum);

		list 
			=	qi::char_('[')[a_list_start(context)] >> *value >> qi::char_(']')[a_list_end(context)];

		string
			=	qi::lexeme[qi::lit("(\xfe\xff") >> 
					*(
						(qi::char_('\0')[a_char(context)] >> escape)
					|	(qi::char_ - ')')[a_char(context)]
					 )
					>> 
					')'];

		escape
			=	('\x5c' > qi::char_("nrtbf()\\")[a_char(context)]) | qi::char_[a_char(context)];

		value 
			=	qi::double_[a_numeral(context)] | list | object | string[a_string(context)] | qi::lit("true")[a_bool(true, context)] | qi::lit("false")[a_bool(false, context)];
	}

	struct a_object_start
	{
		pdf_context &c;
		explicit a_object_start(pdf_context& c) : c(c) {}

		void operator()(qi::unused_type, qi::unused_type, qi::unused_type) const
		{
			c.start_object();
		}
	};

	struct a_object_end
	{
		pdf_context &c;
		explicit a_object_end(pdf_context& c) : c(c) {}

		void operator()(qi::unused_type, qi::unused_type, qi::unused_type) const
		{
			c.end_object();
		}
	};
	struct a_list_start
	{
		pdf_context &c;
		explicit a_list_start(pdf_context& c) : c(c) {}

		void operator()(qi::unused_type, qi::unused_type, qi::unused_type) const
		{
			c.start_object();
		}
	};

	struct a_list_end
	{
		pdf_context &c;
		explicit a_list_end(pdf_context& c) : c(c) {}

		void operator()(qi::unused_type, qi::unused_type, qi::unused_type) const
		{
			c.end_object();
		}
	};

	struct a_name
	{
		pdf_context &c;
		explicit a_name(pdf_context& c) : c(c) {}

		void operator()(std::string const& n, qi::unused_type, qi::unused_type) const
		{
			c.set_name(n);
		}
	};

	struct a_numeral
	{
		pdf_context &c;
		explicit a_numeral(pdf_context& c) : c(c) {}

		void operator()(double const& n, qi::unused_type, qi::unused_type) const
		{
			c.set_value(n);
		}
	};

	struct a_string
	{
		pdf_context &c;
		explicit a_string(pdf_context& c) : c(c) {}

		void operator()(qi::unused_type, qi::unused_type, qi::unused_type) const
		{
			c.set_value();
		}
	};

	struct a_bool
	{
		pdf_context &c;
		bool val;
		a_bool(bool val, pdf_context& c) : val(val), c(c) {}

		void operator()(qi::unused_type, qi::unused_type, qi::unused_type) const
		{
			c.set_value(val);
		}
	};

	struct a_char
	{
		pdf_context &c;
		explicit a_char(pdf_context& c) : c(c) {}

		void operator()(std::uint8_t n, qi::unused_type, qi::unused_type) const
		{
			c.add_char(n);
		}
	};
};

bool read_pdf(boost::property_tree::wptree& tree, const std::string& s)
{
	typedef std::string::const_iterator iterator_type;

	pdf_grammar<iterator_type> g;
	bool result = false;
	
	try
	{
		auto it = s.begin();
		result = qi::phrase_parse(it, s.end(), g, qi::space);
		if(result)
			tree.swap(g.context.root);
	}
	catch(std::exception&)
	{
		CASPAR_LOG_CURRENT_EXCEPTION();
	}

	return result;
}

}	//namespace psd
}	//namespace caspar
