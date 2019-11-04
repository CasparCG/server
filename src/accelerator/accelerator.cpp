#include "accelerator.h"

#include "ogl/image/image_mixer.h"
#include "ogl/util/device.h"

#include <boost/property_tree/ptree.hpp>

#include <common/env.h>

#include <core/mixer/image/image_mixer.h>

#include <mutex>

namespace caspar { namespace accelerator {

struct accelerator::impl
{
    std::shared_ptr<ogl::device> ogl_device_;

    impl() {}

    std::unique_ptr<core::image_mixer> create_image_mixer(int channel_id)
    {
        return std::make_unique<ogl::image_mixer>(spl::make_shared_ptr(get_device()), channel_id);
    }

    std::shared_ptr<ogl::device> get_device()
    {
        if (!ogl_device_) {
            ogl_device_.reset(new ogl::device());
        }

        return ogl_device_;
    }
};

accelerator::accelerator()
    : impl_(std::make_unique<impl>())
{
}

accelerator::~accelerator() {}

std::unique_ptr<core::image_mixer> accelerator::create_image_mixer(int channel_id)
{
    return impl_->create_image_mixer(channel_id);
}

std::shared_ptr<accelerator_device> accelerator::get_device() const
{
    return std::dynamic_pointer_cast<accelerator_device>(impl_->get_device());
}

}} // namespace caspar::accelerator
