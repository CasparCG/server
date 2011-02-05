#include "StdAfx.h"

#include "media.h"

#include <core/producer/color/color_producer.h>
#include <core/producer/ffmpeg/ffmpeg_producer.h>
#include <core/producer/flash/cg_producer.h>
#include <core/producer/image/image_producer.h>
#include <core/producer/decklink/decklink_producer.h>
#include <core/producer/silverlight/silverlight_producer.h>

#include <core/consumer/bluefish/bluefish_consumer.h>
#include <core/consumer/decklink/decklink_consumer.h>
#include <core/consumer/ogl/ogl_consumer.h>
#include <core/consumer/oal/oal_consumer.h>
#include <core/consumer/ffmpeg/ffmpeg_consumer.h>
//#include "../producer/image/image_scroll_producer.h"

#include <common/exception/exceptions.h>

#include <boost/assign.hpp>
#include <boost/filesystem.hpp>

using namespace boost::assign;

namespace caspar { namespace protocol { 
	
using namespace core;

safe_ptr<core::frame_producer> create_producer(const std::vector<std::wstring>& params)
{		
	typedef std::function<safe_ptr<core::frame_producer>(const std::vector<std::wstring>&)> factory_t;

	const auto factories = list_of<factory_t>
		(&core::flash::create_ct_producer)
	//	(&image::create_image_scroll_producer)
		(&core::ffmpeg::create_ffmpeg_producer)
		(&core::image::create_image_producer)
		(&core::create_decklink_producer)
		(&core::create_color_producer)
		//(&core::create_silverlight_producer)
		;

	if(params.empty())
		BOOST_THROW_EXCEPTION(invalid_argument() << arg_name_info("params") << arg_value_info(""));

	auto producer = frame_producer::empty();
	std::any_of(factories.begin(), factories.end(), [&](const factory_t& factory) -> bool
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

	return producer;
}

safe_ptr<core::frame_consumer> create_consumer(const std::vector<std::wstring>& params)
{		
	typedef std::function<safe_ptr<core::frame_consumer>(const std::vector<std::wstring>&)> factory_t;

	const auto factories = list_of<factory_t>
		(&core::create_bluefish_consumer)
		(&core::create_decklink_consumer)
		(&core::create_oal_consumer)
		(&core::create_ogl_consumer)
		(&core::create_ffmpeg_consumer);

	if(params.empty())
		BOOST_THROW_EXCEPTION(invalid_argument() << arg_name_info("params") << arg_value_info(""));

	auto consumer = frame_consumer::empty();
	std::any_of(factories.begin(), factories.end(), [&](const factory_t& factory) -> bool
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

	return consumer;
}

}}
