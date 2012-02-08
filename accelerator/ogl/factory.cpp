#include "../StdAfx.h"

#include "factory.h"

#include "image/image_mixer.h"
#include "util/context.h"

namespace caspar { namespace accelerator { namespace ogl {
	
struct factory::impl
{
	spl::shared_ptr<context> ogl_;
	
	spl::shared_ptr<core::image_mixer> create_image_mixer()
	{
		return spl::make_shared<ogl::image_mixer>(ogl_);
	}
};

factory::factory(){}
spl::shared_ptr<core::image_mixer> factory::create_image_mixer(){return impl_->create_image_mixer();}

}}}