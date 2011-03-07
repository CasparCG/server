#include "../StdAfx.h"

#include "frame_producer.h"

#include <common/memory/safe_ptr.h>

#include <tbb/spin_rw_mutex.h>

namespace caspar { namespace core {
	
std::vector<const producer_factory_t> p_factories;
tbb::spin_rw_mutex p_factories_mutex;

void register_producer_factory(const producer_factory_t& factory)
{
	tbb::spin_rw_mutex::scoped_lock(p_factories_mutex, true);
	p_factories.push_back(factory);
}

safe_ptr<core::frame_producer> create_producer(const std::vector<std::wstring>& params)
{
	if(params.empty())
		BOOST_THROW_EXCEPTION(invalid_argument() << arg_name_info("params") << arg_value_info(""));
	
	tbb::spin_rw_mutex::scoped_lock(p_factories_mutex, false);
	auto producer = frame_producer::empty();
	std::any_of(p_factories.begin(), p_factories.end(), [&](const producer_factory_t& factory) -> bool
		{
			try
			{
				producer = factory(params);
			}
			catch(...)
			{
				CASPAR_LOG_CURRENT_EXCEPTION();
			}
			return producer != frame_producer::empty();
		});

	if(producer == frame_producer::empty())
		BOOST_THROW_EXCEPTION(file_not_found() << msg_info("No match found for supplied commands. Check syntax."));

	return producer;
}

}}