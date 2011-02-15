#pragma once

#include <boost/property_tree/ptree.hpp>

namespace caspar{

struct env
{
	static void initialize(const std::string& filename);
	static const std::wstring& media_folder();
	static const std::wstring& log_folder();
	static const std::wstring& template_folder();
	static const std::wstring& template_host();
	static const std::wstring& data_folder();
	static const std::wstring& version();

	static const boost::property_tree::ptree& properties();
};

};