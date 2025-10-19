#include "accelerator.h"

#include "ogl/image/image_mixer.h"
#include "ogl/util/device.h"

#include "vulkan/image/image_mixer.h"
#include "vulkan/util/device.h"

#include <boost/property_tree/ptree.hpp>

#include <common/bit_depth.h>
#include <common/except.h>

#include <core/mixer/image/image_mixer.h>

#include <memory>
#include <mutex>
#include <utility>

namespace caspar { namespace accelerator {

struct accelerator::impl
{
    std::shared_ptr<accelerator_device> device_;
    const core::video_format_repository format_repository_;
    accelerator_backend                 backend_;

    impl(const core::video_format_repository format_repository)
        : format_repository_(format_repository)
        , backend_(accelerator_backend::invalid)
    {
    }

    void set_backend(accelerator_backend backend)
    {
        if (backend_ != accelerator_backend::invalid) {
            CASPAR_THROW_EXCEPTION(user_error() << msg_info(L"Accelerator backend already set"));
        }

        backend_ = backend;
    }

    std::unique_ptr<core::image_mixer> create_image_mixer(int channel_id, common::bit_depth depth)
    {
        if (backend_ == accelerator_backend::opengl) {
            return std::make_unique<ogl::image_mixer>(
                spl::make_shared_ptr(std::dynamic_pointer_cast<ogl::device>(get_device())),
                channel_id,
                format_repository_.get_max_video_format_size(),
                depth);
        }

        return std::make_unique<vulkan::image_mixer>(
            spl::make_shared_ptr(std::dynamic_pointer_cast<vulkan::device>(get_device())),
            channel_id,
            format_repository_.get_max_video_format_size(),
            depth);
    }

    std::shared_ptr<accelerator_device> get_device()
    {
        if (backend_ == accelerator_backend::invalid) {
            CASPAR_THROW_EXCEPTION(user_error() << msg_info(L"Accelerator backend not set"));
        }

        if (backend_ == accelerator_backend::opengl) {
            if (!device_) {
                device_ = std::dynamic_pointer_cast<accelerator_device>(std::make_shared<ogl::device>());
            }

            return device_;
        }

        if (!device_) {
            device_ = std::dynamic_pointer_cast<accelerator_device>(std::make_shared<vulkan::device>());
        }

        return device_;
    }
};

accelerator::accelerator(const core::video_format_repository format_repository)
    : impl_(std::make_unique<impl>(format_repository))
{
}

accelerator::~accelerator() {}

void accelerator::set_backend(accelerator_backend backend) { impl_->set_backend(backend); }

std::unique_ptr<core::image_mixer> accelerator::create_image_mixer(const int channel_id, common::bit_depth depth)
{
    return impl_->create_image_mixer(channel_id, depth);
}

std::shared_ptr<accelerator_device> accelerator::get_device() const { return impl_->get_device(); }

}} // namespace caspar::accelerator
