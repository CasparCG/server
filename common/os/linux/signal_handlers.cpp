#include "../general_protection_fault.h"

#include "../../except.h"
#include "../../log.h"

#include <signal.h>
#include <pthread.h>

namespace caspar {

thread_local bool has_gpf_handler = false;

void install_gpf_handler()
{
	ensure_gpf_handler_installed_for_thread();
}

void ensure_gpf_handler_installed_for_thread(const char* thread_description)
{
	static const int MAX_LINUX_THREAD_NAME_LEN = 15;
	static auto install = []() { do_install_handlers(); return 0; } ();

	if (has_gpf_handler) {
		return;
	}

	has_gpf_handler = true;

	std::string kernel_thread_name = thread_description;

	if (kernel_thread_name.length() > MAX_LINUX_THREAD_NAME_LEN)
		kernel_thread_name.resize(MAX_LINUX_THREAD_NAME_LEN);

	pthread_setname_np(pthread_self(), kernel_thread_name.c_str());
}

}
