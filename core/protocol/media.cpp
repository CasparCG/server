#include "../StdAfx.h"

#include "media.h"

#include "../producer/color/color_producer.h"
#include "../producer/ffmpeg/ffmpeg_producer.h"
#include "../producer/flash/flash_producer.h"
#include "../producer/flash/ct_producer.h"
#include "../producer/image/image_producer.h"
#include "../producer/image/image_scroll_producer.h"

#include "../../common/exception/exceptions.h"

#include <boost/assign.hpp>
#include <boost/filesystem.hpp>

using namespace boost::assign;

namespace caspar { 
	
frame_producer_ptr load_media(const std::vector<std::wstring>& params, const frame_format_desc& format_desc)
{		
	typedef std::function<frame_producer_ptr(const std::vector<std::wstring>&, const frame_format_desc&)> ProducerFactory;

	const auto producerFactories = list_of<ProducerFactory>
		(&flash::create_flash_producer)
		(&flash::create_ct_producer)
		(&caspar::image::create_image_producer)
		(&caspar::image::create_image_scroll_producer)
		(&ffmpeg::create_ffmpeg_producer)
		(&create_color_producer);

	if(params.empty())
		BOOST_THROW_EXCEPTION(invalid_argument() << arg_name_info("params") << arg_value_info(""));

	frame_producer_ptr pProducer;
	std::any_of(producerFactories.begin(), producerFactories.end(), [&](const ProducerFactory& producerFactory) -> bool
		{
			try
			{
				pProducer = producerFactory(params, format_desc);
			}
			catch(...)
			{
				CASPAR_LOG_CURRENT_EXCEPTION();
			}
			return pProducer != nullptr;
		});

	return pProducer;
}

}
