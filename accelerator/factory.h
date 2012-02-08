#pragma once

#include <common/forward.h>
#include <common/spl/memory.h>

#include <boost/noncopyable.hpp>

FORWARD2(caspar, core, struct image_mixer);

namespace caspar { namespace accelerator {
	
struct factory : boost::noncopyable
{
	virtual spl::shared_ptr<core::image_mixer> create_image_mixer() = 0;
};

}}