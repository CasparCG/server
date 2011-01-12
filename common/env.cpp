#include "stdafx.h"

#include "env.h"

#include "utility/string_convert.h"

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/filesystem.hpp>
#include <boost/thread/once.hpp>

#include <functional>

namespace caspar
{

std::wstring media;
std::wstring log;
std::wstring ftemplate;
std::wstring data;
boost::property_tree::ptree pt;

void do_setup()
{
	std::string initialPath = boost::filesystem::initial_path().file_string();
	
	boost::property_tree::read_xml(initialPath + "\\caspar.config", pt);

	auto paths = pt.get_child("configuration.paths");
	media = widen(paths.get("media-path", initialPath + "\\media\\"));
	log = widen(paths.get("log-path", initialPath + "\\log\\"));
	ftemplate = widen(paths.get("template-path", initialPath + "\\template\\"));
	data = widen(paths.get("data-path", initialPath + "\\data\\"));
}

void setup()
{
	static boost::once_flag setup_flag = BOOST_ONCE_INIT;
	boost::call_once(do_setup, setup_flag);	
}
	
const std::wstring& env::media_folder()
{
	setup();
	return media;
}

const std::wstring& env::log_folder()
{
	setup();
	return log;
}

const std::wstring& env::template_folder()
{
	setup();
	return ftemplate;
}

const std::wstring& env::data_folder()
{
	setup();
	return data;
}

const std::wstring& env::version()
{
	static std::wstring ver = L"2.0.0.2";
	return ver;
}

const std::wstring& env::version_tag()
{
	static std::wstring tag = L"Experimental";
	return tag;
}

const boost::property_tree::ptree& env::properties()
{
	setup();
	return pt;
}

}