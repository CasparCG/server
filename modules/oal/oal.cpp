#include "oal.h"

#include "consumer/oal_consumer.h"

#include <core/consumer/frame_consumer.h>

namespace caspar
{

void init_oal()
{
	core::register_consumer_factory(create_oal_consumer);
}

}