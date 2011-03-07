#pragma once

#include <FreeImage.h>

#include <memory>
#include <string>

namespace caspar { 

std::shared_ptr<FIBITMAP> load_image(const std::string& filename);
std::shared_ptr<FIBITMAP> load_image(const std::wstring& filename);

}
