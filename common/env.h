#pragma once

#include <boost/property_tree/ptree.hpp>

namespace caspar{

struct env
{
	static const std::wstring& media_folder();
	static const std::wstring& log_folder();
	static const std::wstring& template_folder();		
	static const std::wstring& data_folder();
	static const std::wstring& version();
	static const std::wstring& version_tag();

	static const boost::property_tree::ptree& properties();
};

};