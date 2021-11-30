#include "../thread.h"
#include "../../utf.h"
#include <pthread.h>
#include <sched.h>

namespace caspar {

void set_thread_name(const std::wstring& name) { pthread_setname_np(pthread_self(), u8(name).c_str()); }

void set_thread_realtime_priority() {
    pthread_t handle = pthread_self();
    int policy;
    struct sched_param param;
    if (pthread_getschedparam(handle, &policy, &param) != 0)
        return;
    param.sched_priority = 49;
    pthread_setschedparam(handle, SCHED_RR, &param);
}


} // namespace caspar
