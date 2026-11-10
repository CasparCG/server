#include "../thread.h"
#include "../../utf.h"
#include <pthread.h>
#include <sched.h>

namespace caspar {

void set_thread_name(const std::wstring& name)
{
#ifdef __APPLE__
    // macOS can only name the calling thread
    pthread_setname_np(u8(name).c_str());
#else
    pthread_setname_np(pthread_self(), u8(name).c_str());
#endif
}

void set_thread_realtime_priority()
{
#ifdef __APPLE__
    // On macOS sched_priority 2 is a very *low* priority (range ~0-63, default 31), so the Linux
    // code below would deprioritize the channel / output threads. Use the highest QoS class instead,
    // which also keeps the thread on the performance cores.
    pthread_set_qos_class_self_np(QOS_CLASS_USER_INTERACTIVE, 0);
    return;
#endif
    pthread_t          handle = pthread_self();
    int                policy;
    struct sched_param param;
    if (pthread_getschedparam(handle, &policy, &param) != 0)
        return;
    param.sched_priority = 2;
    pthread_setschedparam(handle, SCHED_FIFO, &param);
}

} // namespace caspar
