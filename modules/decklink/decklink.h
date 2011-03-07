#pragma once

#include <string>
#include <vector>

namespace caspar {

void init_decklink();

std::wstring get_decklink_version();
std::vector<std::wstring> get_decklink_device_list();

}