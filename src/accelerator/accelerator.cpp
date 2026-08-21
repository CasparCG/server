#include "accelerator.h"

#include "ogl/image/image_mixer.h"
#include "ogl/util/device.h"

#ifdef ENABLE_VULKAN
#include "vulkan/image/image_mixer.h"
#include "vulkan/util/device.h"
#endif

#include <boost/property_tree/ptree.hpp>

#include <common/bit_depth.h>
#include <common/except.h>
#include <common/log.h>

#include <core/mixer/image/image_mixer.h>

#include <memory>
#include <mutex>
#include <utility>
#include <vector>

namespace caspar { namespace accelerator {

struct accelerator::impl
{
    std::shared_ptr<accelerator_device> device_;
    const core::video_format_repository format_repository_;
    accelerator_backend                 backend_;
#ifdef ENABLE_VULKAN
    std::vector<vulkan_requirements_fn> pending_vulkan_requirements_;
#endif

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

#ifdef ENABLE_VULKAN
    void add_vulkan_requirements(vulkan_requirements_fn fn)
    {
        if (device_) {
            CASPAR_LOG(warning) << L"Vulkan requirements registered after device creation; will not take effect.";
        }
        pending_vulkan_requirements_.push_back(std::move(fn));
    }
#endif

    std::unique_ptr<core::image_mixer> create_image_mixer(int channel_id, common::bit_depth depth)
    {
#ifdef ENABLE_VULKAN
        if (backend_ == accelerator_backend::vulkan) {
            return std::make_unique<vulkan::image_mixer>(
                spl::make_shared_ptr(std::dynamic_pointer_cast<vulkan::device>(get_device())),
                channel_id,
                format_repository_.get_max_video_format_size(),
                depth);
        }
#endif
        return std::make_unique<ogl::image_mixer>(
            spl::make_shared_ptr(std::dynamic_pointer_cast<ogl::device>(get_device())),
            channel_id,
            format_repository_.get_max_video_format_size(),
            depth);
    }

    std::shared_ptr<accelerator_device> get_device()
    {
        if (backend_ == accelerator_backend::invalid) {
            CASPAR_THROW_EXCEPTION(user_error() << msg_info(L"Accelerator backend not set"));
        }
#ifdef ENABLE_VULKAN
        if (backend_ == accelerator_backend::vulkan) {
            if (!device_) {
                device_ = std::dynamic_pointer_cast<accelerator_device>(
                    std::make_shared<vulkan::device>(pending_vulkan_requirements_));
            }

            return device_;
        }
#endif

        if (!device_) {
            device_ = std::dynamic_pointer_cast<accelerator_device>(std::make_shared<ogl::device>());
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

#ifdef ENABLE_VULKAN
void accelerator::add_vulkan_requirements(vulkan_requirements_fn fn) { impl_->add_vulkan_requirements(std::move(fn)); }
#endif

std::unique_ptr<core::image_mixer> accelerator::create_image_mixer(const int channel_id, common::bit_depth depth)
{
    return impl_->create_image_mixer(channel_id, depth);
}

std::shared_ptr<accelerator_device> accelerator::get_device() const { return impl_->get_device(); }

}} // namespace caspar::accelerator
