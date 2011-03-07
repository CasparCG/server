#include "image.h"

#include "producer/image_producer.h"

#include <core/producer/frame_producer.h>

#include <common/utility/string.h>

#include <FreeImage.h>

namespace caspar
{

void init_image()
{
	core::register_producer_factory(create_image_producer);
}

std::wstring get_image_version()
{
	return widen(std::string(FreeImage_GetVersion()));
}

}