#include "bluefish.h"

#include <core/consumer/frame_consumer.h>

namespace caspar {

void init_bluefish()
{
	try
	{
		blue_initialize();
	}
	catch(...){}
	core::register_consumer_factory(create_bluefish_consumer);
}

}