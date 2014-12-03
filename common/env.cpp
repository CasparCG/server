/*
* Copyright 2013 Sveriges Television AB http://casparcg.com/
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

namespace fs = boost::filesystem;

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
		auto initialPath = fs::initial_path<fs::path>().wstring();
	
		std::wifstream file(initialPath + L"\\" + filename);
		boost::property_tree::read_xml(file, pt, boost::property_tree::xml_parser::trim_whitespace | boost::property_tree::xml_parser::no_comments);

		auto paths = pt.get_child(L"configuration.paths");
		media = widen(paths.get(L"media-path", initialPath + L"\\media\\"));
		log = widen(paths.get(L"log-path", initialPath + L"\\log\\"));
		ftemplate = fs::complete(fs::path(widen(paths.get(L"template-path", initialPath + L"\\template\\")))).wstring();		
		data = widen(paths.get(L"data-path", initialPath + L"\\data\\"));
		thumbnails = widen(paths.get(L"thumbnails-path", initialPath + L"\\thumbnails\\"));

		//Make sure that all paths have a trailing backslash
		if(media.at(media.length()-1) != L'\\')
			media.append(L"\\");
		if(log.at(log.length()-1) != L'\\')
			log.append(L"\\");
		if(ftemplate.at(ftemplate.length()-1) != L'\\')
			ftemplate.append(L"\\");
		if(data.at(data.length()-1) != L'\\')
			data.append(L"\\");
		if(thumbnails.at(thumbnails.length()-1) != L'\\')
			thumbnails.append(L"\\");

		try
		{
			for(auto it = fs::directory_iterator(initialPath); it != fs::directory_iterator(); ++it)
			{
				if(it->path().filename().wstring().find(L".fth") != std::wstring::npos)			
				{
					auto from_path = *it;
					auto to_path = fs::path(ftemplate + L"/" + it->path().filename().wstring());
				
					if(fs::exists(to_path))
						fs::remove(to_path);

					fs::copy_file(from_path, to_path);
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
		auto media_path = fs::path(media);
		if(!fs::exists(media_path))
			fs::create_directory(media_path);
		
		auto log_path = fs::path(log);
		if(!fs::exists(log_path))
			fs::create_directory(log_path);
		
		auto template_path = fs::path(ftemplate);
		if(!fs::exists(template_path))
			fs::create_directory(template_path);
		
		auto data_path = fs::path(data);
		if(!fs::exists(data_path))
			fs::create_directory(data_path);
		
		auto thumbnails_path = fs::path(thumbnails);
		if(!fs::exists(thumbnails_path))
			fs::create_directory(thumbnails_path);
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
			CASPAR_REV	" " 
			CASPAR_TAG);
	return ver;
}

const boost::property_tree::wptree& properties()
{
	check_is_configured();
	return pt;
}

}}