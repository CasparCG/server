#pragma once

#include <string>

namespace caspar {

void set_thread_name(const std::wstring& name);
void set_thread_realtime_priority();
}


