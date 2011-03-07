#include "stdafx.h"

#include "silverlight.h"

#include "producer/silverlight_producer.h"

#include <core/producer/frame_producer.h>

namespace caspar{

void init_silverlight()
{
	core::register_producer_factory(create_silverlight_producer);
}

}