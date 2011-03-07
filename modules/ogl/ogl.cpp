#include "ogl.h"

#include "consumer/ogl_consumer.h"

#include <core/consumer/frame_consumer.h>

namespace caspar
{

void init_ogl()
{
	core::register_consumer_factory(create_ogl_consumer);
}

}