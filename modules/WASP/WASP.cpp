#include "WASP.h"

#include "producer/WASP_producer.h"
//#include "producer/image_scroll_producer.h"
//#include "producer/WASP_consumer.h"

#include <core/parameters/parameters.h>
#include <core/producer/frame_producer.h>
#include <core/consumer/frame_consumer.h>

//#include <common/utility/string.h>

//#include <FreeImage.h>

namespace caspar { namespace WASP {

void init()
{
	//core::register_producer_factory(create_scroll_producer);
	core::register_producer_factory(create_producer);
	core::register_thumbnail_producer_factory(create_thumbnail_producer);
}

std::wstring get_version()
{
	//return widen(std::string(FreeImage_GetVersion()));
	return L"WASP 2014";
}

}}