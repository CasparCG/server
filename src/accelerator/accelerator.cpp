#include "accelerator.h"

#include "ogl/image/image_mixer.h"
#include "ogl/util/device.h"

#include <boost/property_tree/ptree.hpp>

#include <core/mixer/image/image_mixer.h>

#include <memory>
#include <mutex>
#include <utility>

namespace caspar { namespace accelerator {

struct accelerator::impl
{
    std::shared_ptr<ogl::device>        ogl_device_;
    const core::video_format_repository format_repository_;

    impl(const core::video_format_repository format_repository)
        : format_repository_(format_repository)
    {
    }

    std::unique_ptr<core::image_mixer> create_image_mixer(const int channel_id)
    {
        return std::make_unique<ogl::image_mixer>(
            spl::make_shared_ptr(get_device()), channel_id, format_repository_.get_max_video_format_size());
    }

    std::shared_ptr<ogl::device> get_device()
    {
        if (!ogl_device_) {
            ogl_device_ = std::make_shared<ogl::device>();
        }

        return ogl_device_;
    }
};

accelerator::accelerator(const core::video_format_repository format_repository)
    : impl_(std::make_unique<impl>(format_repository))
{
}

accelerator::~accelerator() {}

std::unique_ptr<core::image_mixer> accelerator::create_image_mixer(const int channel_id)
{
    return impl_->create_image_mixer(channel_id);
}

std::shared_ptr<accelerator_device> accelerator::get_device() const
{
    return std::dynamic_pointer_cast<accelerator_device>(impl_->get_device());
}

}} // namespace caspar::accelerator
