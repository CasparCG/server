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

#include <memory>
#include "descriptor.h"
#include <boost\property_tree\ptree.hpp>
#include "misc.h"

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

	descriptor::descriptor() : context_(std::make_shared<context>())
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

bool descriptor::populate(BEFileInputStream& stream)
{
	bool result = true;

	try
	{
		stream.read_unicode_string();
		stream.read_id_string();
		unsigned long element_count = stream.read_long();
		for(int element_index = 0; element_index < element_count; ++element_index)
		{
			std::wstring key = stream.read_id_string();
			read_value(key, stream);
		}
	}
	catch(std::exception& ex)
	{
		result = false;
	}

	return result;
}

void descriptor::read_value(const std::wstring& key, BEFileInputStream& stream)
{
	unsigned int type = stream.read_long();

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
			context_->stack.back()->put(stream.read_id_string(), stream.read_id_string());
		}
		break;

	case 'long':
		{
			context_->stack.back()->put(key, stream.read_long());
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
			unsigned long count = stream.read_long();
			for(int i = 0; i < count; ++i)
			{
				read_value(L"", stream);
			}
		}
		break;

	case 'tdta':
		{
			unsigned long rawdata_length = stream.read_long();
			std::vector<char> rawdata(rawdata_length);
			stream.read(rawdata.data(), rawdata_length);

			std::wstring data_str(rawdata.begin(), rawdata.end());
			context_->stack.back()->put(key, data_str);
		}
		break;

	case 'UntF': 
		{
			context::scoped_holder list(key, context_);
			unsigned long unit = stream.read_long();
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
		throw PSDFileFormatException();
	}
}

}	//namespace psd
}	//namespace caspar