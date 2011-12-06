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

#include "exception\exceptions.h"
#include "utility/string.h"

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/filesystem.hpp>
#include <boost/thread/once.hpp>

#include <functional>
#include <iostream>

namespace caspar { namespace env {
	
std::string media;
std::string log;
std::string ftemplate;
std::string data;
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
		auto initialPath = boost::filesystem::initial_path().file_string();
	
		std::ifstream file(initialPath + "\\" + filename);
		boost::property_tree::read_xml(file, pt, boost::property_tree::xml_parser::trim_whitespace | boost::property_tree::xml_parser::no_comments);

		auto paths = pt.get_child("configuration.paths");
		media = paths.get("media-path", initialPath + "\\media\\");
		log = paths.get("log-path", initialPath + "\\log\\");
		ftemplate = boost::filesystem2::complete(paths.get("template-path", initialPath + "\\template\\")).string();		
		data = paths.get("data-path", initialPath + "\\data\\");
	}
	catch(...)
	{
		std::wcout << " ### Invalid configuration file. ###";
		throw;
	}
}
	
const std::string& media_folder()
{
	check_is_configured();
	return media;
}

const std::string& log_folder()
{
	check_is_configured();
	return log;
}

const std::string& template_folder()
{
	check_is_configured();
	return ftemplate;
}

const std::string& data_folder()
{
	check_is_configured();
	return data;
}

const std::string& version()
{
	static std::string ver = std::string("") + CASPAR_GEN + "." + CASPAR_MAYOR + "." + CASPAR_MINOR + "." + CASPAR_REV + " " + CASPAR_TAG;
	return ver;
}

const boost::property_tree::ptree& properties()
{
	check_is_configured();
	return pt;
}

}}