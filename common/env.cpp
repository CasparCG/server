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

#include "exception/exceptions.h"
#include "log/log.h"
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
std::wstring thumbnails;
boost::property_tree::wptree pt;

void check_is_configured()
{
	if(pt.empty())
		BOOST_THROW_EXCEPTION(invalid_operation() << msg_info("Enviroment properties has not been configured"));
}

void configure(const std::wstring& filename)
{
	try
	{
		auto initialPath = boost::filesystem::initial_path<boost::filesystem2::wpath>().file_string();
	
		std::wifstream file(initialPath + L"\\" + filename);
		boost::property_tree::read_xml(file, pt, boost::property_tree::xml_parser::trim_whitespace | boost::property_tree::xml_parser::no_comments);

		auto paths = pt.get_child(L"configuration.paths");
		media = widen(paths.get(L"media-path", initialPath + L"\\media\\"));
		log = widen(paths.get(L"log-path", initialPath + L"\\log\\"));
		ftemplate = complete(wpath(widen(paths.get(L"template-path", initialPath + L"\\template\\")))).string();		
		data = widen(paths.get(L"data-path", initialPath + L"\\data\\"));
		thumbnails = widen(paths.get(L"thumbnails-path", initialPath + L"\\thumbnails\\"));

		try
		{
			for(auto it = boost::filesystem2::wdirectory_iterator(initialPath); it != boost::filesystem2::wdirectory_iterator(); ++it)
			{
				if(it->filename().find(L".fth") != std::wstring::npos)			
				{
					auto from_path = *it;
					auto to_path = boost::filesystem2::wpath(ftemplate + L"/" + it->filename());
				
					if(boost::filesystem2::exists(to_path))
						boost::filesystem2::remove(to_path);

					boost::filesystem2::copy_file(from_path, to_path);
				}	
			}
		}
		catch(...)
		{
			CASPAR_LOG_CURRENT_EXCEPTION();
			CASPAR_LOG(error) << L"Failed to copy template-hosts from initial-path to template-path.";
		}
	}
	catch(...)
	{
		std::wcout << L" ### Invalid configuration file. ###";
		throw;
	}

	try
	{
		auto media_path = boost::filesystem::wpath(media);
		if(!boost::filesystem::exists(media_path))
			boost::filesystem::create_directory(media_path);
		
		auto log_path = boost::filesystem::wpath(log);
		if(!boost::filesystem::exists(log_path))
			boost::filesystem::create_directory(log_path);
		
		auto template_path = boost::filesystem::wpath(ftemplate);
		if(!boost::filesystem::exists(template_path))
			boost::filesystem::create_directory(template_path);
		
		auto data_path = boost::filesystem::wpath(data);
		if(!boost::filesystem::exists(data_path))
			boost::filesystem::create_directory(data_path);
		
		auto thumbnails_path = boost::filesystem::wpath(thumbnails);
		if(!boost::filesystem::exists(thumbnails_path))
			boost::filesystem::create_directory(thumbnails_path);
	}
	catch(...)
	{
		CASPAR_LOG_CURRENT_EXCEPTION();
		CASPAR_LOG(error) << L"Failed to create configured directories.";
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

const std::wstring& thumbnails_folder()
{
	check_is_configured();
	return thumbnails;
}

#define QUOTE(str) #str
#define EXPAND_AND_QUOTE(str) QUOTE(str)

const std::wstring& version()
{
	static std::wstring ver = widen(
			EXPAND_AND_QUOTE(CASPAR_GEN)	"." 
			EXPAND_AND_QUOTE(CASPAR_MAYOR)  "." 
			EXPAND_AND_QUOTE(CASPAR_MINOR)  "." 
			EXPAND_AND_QUOTE(CASPAR_REV)	" " 
			CASPAR_TAG);
	return ver;
}

const boost::property_tree::wptree& properties()
{
	check_is_configured();
	return pt;
}

}}