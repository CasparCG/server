#pragma once

#include <common/bit_depth.h>

#include <core/frame/pixel_format.h>
#include <core/mixer/mixer.h>
#include <core/video_format.h>

#include <boost/property_tree/ptree_fwd.hpp>

#include <functional>
#include <future>
#include <memory>
#include <string>

#ifdef ENABLE_VULKAN
namespace vkb {
struct PhysicalDevice;
}
#endif

namespace caspar { namespace accelerator {

class accelerator_device
{
  public:
    virtual boost::property_tree::wptree info() const = 0;
    virtual std::future<void>            gc()         = 0;
};

enum class accelerator_backend
{
    invalid = 0,
    opengl,
#ifdef ENABLE_VULKAN
    vulkan,
#endif
};

#ifdef ENABLE_VULKAN
using vulkan_requirements_fn = std::function<void(vkb::PhysicalDevice&)>;
#endif

class accelerator
{
  public:
    explicit accelerator(const core::video_format_repository format_repository);
    accelerator(accelerator&) = delete;
    ~accelerator();

    accelerator& operator=(accelerator&) = delete;

    void set_backend(accelerator_backend backend);

#ifdef ENABLE_VULKAN
    void add_vulkan_requirements(vulkan_requirements_fn fn);
#endif

    std::unique_ptr<caspar::core::image_mixer> create_image_mixer(int channel_id, common::bit_depth depth);

    std::shared_ptr<accelerator_device> get_device() const;

  private:
    struct impl;
    std::unique_ptr<impl> impl_;
};

}} // namespace caspar::accelerator
