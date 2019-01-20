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
    const std::wstring           path_;
    std::mutex                   mutex_;
    std::shared_ptr<ogl::device> ogl_device_;

    impl(std::wstring path)
        : path_(std::move(path))
    {
    }

    std::unique_ptr<core::image_mixer> create_image_mixer(int channel_id)
    {
        if (!ogl_device_) {
            ogl_device_ = std::make_shared<ogl::device>();
        }

        return std::make_unique<ogl::image_mixer>(spl::make_shared_ptr(ogl_device_), channel_id);
    }
};

accelerator::accelerator(const std::wstring& path)
    : impl_(std::make_unique<impl>(path))
{
}

accelerator::~accelerator() {}

std::unique_ptr<core::image_mixer> accelerator::create_image_mixer(int channel_id)
{
    return impl_->create_image_mixer(channel_id);
}

}} // namespace caspar::accelerator
