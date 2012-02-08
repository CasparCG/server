#pragma once

#include "../factory.h"

#include <common/spl/memory.h>

namespace caspar { namespace accelerator { namespace ogl {
	
class factory : public accelerator::factory
{
public:
	factory();

	virtual spl::shared_ptr<core::image_mixer> create_image_mixer() override;
private:
	struct impl;
	spl::shared_ptr<impl> impl_;
};

}}}