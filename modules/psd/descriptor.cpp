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

#include "descriptor.h"
#include "misc.h"
#include "util/pdf_reader.h"

#include <common/log.h>

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/locale.hpp>

#include <memory>

namespace caspar { namespace psd {

	class descriptor::context::scoped_holder
	{
		descriptor::context::ptr_type ctx_;
	public:
		explicit scoped_holder(const std::wstring& key, descriptor::context::ptr_type ctx) : ctx_(ctx) 
		{
			Ptree *parent = ctx_->stack.back();
			Ptree *child = &parent->push_back(std::make_pair(key, Ptree()))->second;
			ctx_->stack.push_back(child);
		}
		~scoped_holder()
		{
			ctx_->stack.pop_back();
		}
	};

	descriptor::descriptor(const std::wstring& debug_name) : context_(std::make_shared<context>(debug_name))
	{
		context_->stack.push_back(&context_->root);
	}
	descriptor::descriptor(const std::wstring& key, context::ptr_type context) : context_(context)
	{
		Ptree *parent = context_->stack.back();
		Ptree *child = &parent->push_back(std::make_pair(key, Ptree()))->second;
		context->stack.push_back(child);
	}
	descriptor::~descriptor()
	{
		context_->stack.pop_back();
	}

void descriptor::populate(bigendian_file_input_stream& stream)
{
	stream.read_unicode_string();
	stream.read_id_string();
	auto element_count = stream.read_long();
	for (std::uint32_t element_index = 0; element_index < element_count; ++element_index)
	{
		std::wstring key = stream.read_id_string();
		read_value(key, stream);
	}

	if (context_->stack.size() == 1)
		log::print_child(boost::log::trivial::trace, context_->debug_name + L": ", L"", context_->root);
}

void descriptor::read_value(const std::wstring& key, bigendian_file_input_stream& stream)
{
	auto type = stream.read_long();

	switch(type)
	{
	case 'Objc': 
		{
			descriptor desc(key, context_);
			desc.populate(stream);
		}
		break;

	case 'doub': 
		{
			context_->stack.back()->put(key, stream.read_double());
		}
		break;

	case 'TEXT':
		{
			context_->stack.back()->put(key, stream.read_unicode_string());
		}
		break;

	case 'enum':
		{
			auto k = stream.read_id_string();
			auto v = stream.read_id_string();
			context_->stack.back()->put(k, v);
		}
		break;

	case 'long':
		{
			std::int32_t value = stream.read_long();
			context_->stack.back()->put(key, value);
		}
		break;

	case 'bool': 
		{
			context_->stack.back()->put(key, stream.read_byte());
		}
		break;

	case 'VlLs': 
		{
			context::scoped_holder list(key, context_);
			auto count = stream.read_long();
			for(std::uint32_t i = 0; i < count; ++i)
			{
				read_value(L"li", stream);
			}
		}
		break;

	case 'tdta':
		{
			auto rawdata_length = stream.read_long();
			std::vector<char> rawdata(rawdata_length);
			stream.read(rawdata.data(), rawdata_length);
			std::wstring data_str(rawdata.begin(), rawdata.end());
			context_->stack.back()->put(key, data_str);
		}
		break;

	case 'UntF': 
		{
			context::scoped_holder list(key, context_);
			auto unit = stream.read_long();
			std::string type(reinterpret_cast<char*>(&unit), 4);
			std::wstring wtype(type.rbegin(), type.rend());
			context_->stack.back()->put(wtype, stream.read_double());
		}
		break;

	case 'obj ': 
	case 'GlbO': 
	case 'type':
	case 'GlbC':
	case 'alis':
	default:
		//descriptor type not supported yet
		CASPAR_THROW_EXCEPTION(psd_file_format_exception() << msg_info("descriptor type not supported yet"));
	}
}

}	//namespace psd
}	//namespace caspar
