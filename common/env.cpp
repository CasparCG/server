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

#include "stdafx.h"

#include "env.h"

#include "../version.h"

#include "log/log.h"
#include "exception/exceptions.h"
#include "utility/string.h"

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/filesystem.hpp>
#include <boost/thread/once.hpp>

#include <functional>
#include <iostream>

namespace caspar { namespace env {
	
std::wstring media;
std::wstring log;
std::wstring ftemplate;
std::wstring data;
boost::property_tree::wptree pt;

void check_is_configured()
{
	if(pt.empty())
		BOOST_THROW_EXCEPTION(invalid_operation() << wmsg_info(L"Enviroment properties has not been configured"));
}

void configure(const std::wstring& filename)
{
	try
	{
		auto initialPath = boost::filesystem2::initial_path<boost::filesystem2::wpath>().file_string();
	
		std::wifstream file(initialPath + L"\\" + filename);
		boost::property_tree::read_xml(file, pt, boost::property_tree::xml_parser::trim_whitespace | boost::property_tree::xml_parser::no_comments);

		auto paths	= pt.get_child(L"configuration.paths");
		media		= paths.get(L"media-path", initialPath + L"\\media\\");
		log			= paths.get(L"log-path", initialPath + L"\\log\\");
		ftemplate	= boost::filesystem2::complete(paths.get(L"template-path", initialPath + L"\\template\\")).string();		
		data		= paths.get(L"data-path", initialPath + L"\\data\\");
	}
	catch(...)
	{
		CASPAR_LOG(error) << L" ### Invalid configuration file. ###";
		throw;
	}
}
	
const std::wstring& media_folder()
{
	check_is_configured();
	return media;
}

const std::wstring& log_folder()
{
	check_is_configured();
	return log;
}

const std::wstring& template_folder()
{
	check_is_configured();
	return ftemplate;
}

const std::wstring& data_folder()
{
	check_is_configured();
	return data;
}

const std::wstring& version()
{
	static std::wstring ver = std::wstring(L"") + CASPAR_GEN + L"." + CASPAR_MAYOR + L"." + CASPAR_MINOR + L"." + CASPAR_REV + L" " + CASPAR_TAG;
	return ver;
}

const boost::property_tree::wptree& properties()
{
	check_is_configured();
	return pt;
}

}}