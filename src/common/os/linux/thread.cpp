#include "../thread.h"
#include "../../utf.h"

namespace caspar {

void set_thread_name(const std::wstring& name) { pthread_setname_np(pthread_self(), u8(name).c_str()); }

} // namespace caspar
