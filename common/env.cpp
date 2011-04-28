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

namespace caspar { namespace env {

std::wstring media;
std::wstring log;
std::wstring ftemplate;
std::wstring ftemplate_host;
std::wstring data;
boost::property_tree::ptree pt;

void check_is_configured()
{
	if(pt.empty())
		BOOST_THROW_EXCEPTION(invalid_operation() << msg_info("Enviroment properties has not been configured"));
}

void configure(const std::string& filename)
{
	std::string initialPath = boost::filesystem::initial_path().file_string();
	
	boost::property_tree::read_xml(initialPath + "\\" + filename, pt);

	auto paths = pt.get_child("configuration.paths");
	media = widen(paths.get("media-path", initialPath + "\\media\\"));
	log = widen(paths.get("log-path", initialPath + "\\log\\"));
	ftemplate = widen(paths.get("template-path", initialPath + "\\template\\"));
	ftemplate_host = widen(paths.get("template-host", "cg.fth"));
	data = widen(paths.get("data-path", initialPath + "\\data\\"));
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

const std::wstring& template_host()
{
	check_is_configured();
	return ftemplate_host;
}

const std::wstring& data_folder()
{
	check_is_configured();
	return data;
}

const std::wstring& version()
{
	static std::wstring ver = std::wstring(L"") + CASPAR_GEN + L"." + CASPAR_MAYOR + L"." + CASPAR_MINOR + L"." + CASPAR_REV;
	return ver;
}

const boost::property_tree::ptree& properties()
{
	check_is_configured();
	return pt;
}

}}