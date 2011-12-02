#pragma once

#include <string>

#include <boost/property_tree/ptree_fwd.hpp>

namespace caspar { namespace flash {

std::string read_template_meta_info(const std::wstring& filename);

}}