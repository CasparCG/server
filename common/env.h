#pragma once

#include <boost/property_tree/ptree.hpp>

#include <string>

namespace caspar { namespace env {

void configure(const std::string& filename);

const std::wstring& media_folder();
const std::wstring& log_folder();
const std::wstring& template_folder();
const std::wstring& template_host();
const std::wstring& data_folder();
const std::wstring& version();

const boost::property_tree::ptree& properties();

} }