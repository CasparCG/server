#pragma once

#include <string>
#include <vector>

namespace caspar { namespace common {

std::wstring find_file(const std::wstring& filename, const std::vector<std::wstring> extensions);

}}