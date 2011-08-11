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
#include "stdafx.h"

#include "env.h"

#include "../version.h"

#include "exception\exceptions.h"
#include "utility/string.h"

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/filesystem.hpp>
#include <boost/thread/once.hpp>

#include <functional>
#include <iostream>

namespace caspar { namespace env {

using namespace boost::filesystem2;

std::wstring media;
std::wstring log;
std::wstring ftemplate;
std::wstring data;
boost::property_tree::ptree pt;

void check_is_configured()
{
	if(pt.empty())
		BOOST_THROW_EXCEPTION(invalid_operation() << msg_info("Enviroment properties has not been configured"));
}

void configure(const std::string& filename)
{
	try
	{
		std::string initialPath = boost::filesystem::initial_path().file_string();
	
		boost::property_tree::read_xml(initialPath + "\\" + filename, pt);

		auto paths = pt.get_child("configuration.paths");
		media = widen(paths.get("media-path", initialPath + "\\media\\"));
		log = widen(paths.get("log-path", initialPath + "\\log\\"));
		ftemplate = complete(wpath(widen(paths.get("template-path", initialPath + "\\template\\")))).string();		
		data = widen(paths.get("data-path", initialPath + "\\data\\"));
	}
	catch(...)
	{
		std::wcout << L" ### Invalid configuration file. ###";
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
	static std::wstring ver = std::wstring(L"") + CASPAR_GEN + L"." + CASPAR_MAYOR + L"." + CASPAR_MINOR + L"." + CASPAR_REV + L" ALPHA";
	return ver;
}

const boost::property_tree::ptree& properties()
{
	check_is_configured();
	return pt;
}

}}