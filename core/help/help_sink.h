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
* Author: Helge Norberg, helge.norberg@svt.se
*/

#pragma once

#include <string>

#include <common/memory.h>

namespace caspar { namespace core {

class paragraph_builder : public spl::enable_shared_from_this<paragraph_builder>
{
public:
	virtual ~paragraph_builder() { }
	virtual spl::shared_ptr<paragraph_builder> text(std::wstring text) { return shared_from_this(); };
	virtual spl::shared_ptr<paragraph_builder> code(std::wstring text) { return shared_from_this(); };
	virtual spl::shared_ptr<paragraph_builder> strong(std::wstring text) { return shared_from_this(); };
	virtual spl::shared_ptr<paragraph_builder> see(std::wstring item) { return shared_from_this(); };
	virtual spl::shared_ptr<paragraph_builder> url(std::wstring url, std::wstring name = L"") { return shared_from_this(); };
};

class definition_list_builder : public spl::enable_shared_from_this<definition_list_builder>
{
public:
	virtual ~definition_list_builder() { }
	virtual spl::shared_ptr<definition_list_builder> item(std::wstring term, std::wstring description) { return shared_from_this(); };
};

class help_sink
{
public:
	virtual ~help_sink() { }

	virtual void short_description(const std::wstring& short_description) { };
	virtual void syntax(const std::wstring& syntax) { };
	virtual spl::shared_ptr<paragraph_builder> para() { return spl::make_shared<paragraph_builder>(); }
	virtual spl::shared_ptr<definition_list_builder> definitions() { return spl::make_shared<definition_list_builder>(); }
	virtual void example(const std::wstring& code, const std::wstring& caption = L"") { }
private:
	virtual void begin_item(const std::wstring& name) { };
	virtual void end_item() { };

	friend help_repository;
};

}}
