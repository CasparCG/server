#include "../StdAfx.h"

#include "media.h"

#include "../producer/color/color_producer.h"
#include "../producer/ffmpeg/ffmpeg_producer.h"
#include "../producer/flash/flash_producer.h"
#include "../producer/flash/ct_producer.h"
#include "../producer/image/image_producer.h"
//#include "../producer/image/image_scroll_producer.h"

#include "../../common/exception/exceptions.h"

#include <boost/assign.hpp>
#include <boost/filesystem.hpp>

using namespace boost::assign;

namespace caspar { namespace core { 
	
safe_ptr<frame_producer> load_media(const std::vector<std::wstring>& params)
{		
	typedef std::function<safe_ptr<frame_producer>(const std::vector<std::wstring>&)> producer_factory;

	const auto producer_factories = list_of<producer_factory>
		(&flash::create_ct_producer)
		(&image::create_image_producer)
	//	(&image::create_image_scroll_producer)
		(&ffmpeg::create_ffmpeg_producer)
		(&create_color_producer);

	if(params.empty())
		BOOST_THROW_EXCEPTION(invalid_argument() << arg_name_info("params") << arg_value_info(""));

	safe_ptr<frame_producer> producer(frame_producer::empty());
	std::any_of(producer_factories.begin(), producer_factories.end(), [&](const producer_factory& factory) -> bool
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

}}
