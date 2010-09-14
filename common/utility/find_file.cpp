#include "../stdafx.h"

#include "find_file.h"

#include <boost/filesystem.hpp>

#include <string>
#include <algorithm>

namespace caspar { namespace common {

std::wstring find_file(const std::wstring& filename, const std::vector<std::wstring> extensions)
{
	auto it = std::find_if(extensions.cbegin(), extensions.cend(), [&](const std::wstring& extension) -> bool
		{					
			auto filePath = boost::filesystem::wpath(filename);
			filePath.replace_extension(extension);
			return boost::filesystem::exists(filePath) && boost::filesystem::is_regular_file(filePath);
		});

	return it != extensions.end() ? filename + L"." + *it : L"";
}

}}