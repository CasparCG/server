#include "../StdAfx.h"

#include "factory.h"

#include "image/image_mixer.h"

namespace caspar { namespace accelerator { namespace cpu {
	
spl::shared_ptr<core::image_mixer> factory::create_image_mixer()
{
	return spl::shared_ptr<cpu::image_mixer>();
}

}}}