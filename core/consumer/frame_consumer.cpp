#include "../StdAfx.h"

#include "frame_consumer.h"

#include <common/memory/safe_ptr.h>

#include <tbb/spin_rw_mutex.h>

namespace caspar { namespace core {
	
std::vector<const consumer_factory_t> c_factories;
tbb::spin_rw_mutex c_factories_mutex;

void register_consumer_factory(const consumer_factory_t& factory)
{
	tbb::spin_rw_mutex::scoped_lock(c_factories_mutex, true);
	c_factories.push_back(factory);
}

safe_ptr<core::frame_consumer> create_consumer(const std::vector<std::wstring>& params)
{
	if(params.empty())
		BOOST_THROW_EXCEPTION(invalid_argument() << arg_name_info("params") << arg_value_info(""));
	
	tbb::spin_rw_mutex::scoped_lock(c_factories_mutex, false);
	auto consumer = frame_consumer::empty();
	std::any_of(c_factories.begin(), c_factories.end(), [&](const consumer_factory_t& factory) -> bool
		{
			try
			{
				consumer = factory(params);
			}
			catch(...)
			{
				CASPAR_LOG_CURRENT_EXCEPTION();
			}
			return consumer != frame_consumer::empty();
		});

	if(consumer == frame_consumer::empty())
		BOOST_THROW_EXCEPTION(file_not_found() << msg_info("No match found for supplied commands. Check syntax."));

	return consumer;
}

}}