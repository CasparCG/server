#include "../general_protection_fault.h"

#include "../../except.h"
#include "../../log.h"
#include "../../thread_info.h"

#include <signal.h>
#include <pthread.h>

namespace caspar {

struct floating_point_exception : virtual caspar_exception {};
struct segmentation_fault_exception : virtual caspar_exception {};

void catch_fpe(int signum)
{
	try
	{
		CASPAR_THROW_EXCEPTION(floating_point_exception());
	}
	catch (...)
	{
		CASPAR_LOG_CURRENT_EXCEPTION();
		throw;
	}
}

void catch_segv(int signum)
{
	try
	{
		CASPAR_THROW_EXCEPTION(segmentation_fault_exception());
	}
	catch (...)
	{
		CASPAR_LOG_CURRENT_EXCEPTION();
		throw;
	}
}

void do_install_handlers()
{
	signal(SIGFPE, catch_fpe);
	signal(SIGSEGV, catch_segv);
}

void install_gpf_handler()
{
	ensure_gpf_handler_installed_for_thread();
}

void ensure_gpf_handler_installed_for_thread(
		const char* thread_description)
{
	static const int MAX_LINUX_THREAD_NAME_LEN = 15;
	static auto install = []() { do_install_handlers(); return 0; } ();

	auto& for_thread = get_thread_info();
	
	if (thread_description && for_thread.name.empty())
	{
		for_thread.name = thread_description;

		if (std::strlen(thread_description) > MAX_LINUX_THREAD_NAME_LEN)
		{
			char truncated[MAX_LINUX_THREAD_NAME_LEN + 1];
			std::memcpy(truncated, thread_description, MAX_LINUX_THREAD_NAME_LEN);
			truncated[MAX_LINUX_THREAD_NAME_LEN] = 0;
			pthread_setname_np(pthread_self(), truncated);
		}
		else
			pthread_setname_np(pthread_self(), thread_description);
	}
}

}
