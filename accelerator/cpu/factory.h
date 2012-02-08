#pragma once

#include "../factory.h"

namespace caspar { namespace accelerator { namespace cpu {
	
class factory : public accelerator::factory
{
public:
	virtual spl::shared_ptr<core::image_mixer> create_image_mixer() override;
};

}}}