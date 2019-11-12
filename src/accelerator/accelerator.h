#pragma once

#include <core/mixer/mixer.h>

#include <boost/property_tree/ptree.hpp>

#include <future>
#include <memory>
#include <string>

namespace caspar { namespace accelerator {

class accelerator_device
{
  public:
    virtual boost::property_tree::wptree info() const = 0;
    virtual std::future<void>            gc()         = 0;
};

class accelerator
{
  public:
    explicit accelerator();
    accelerator(accelerator&) = delete;
    ~accelerator();

    accelerator& operator=(accelerator&) = delete;

    std::unique_ptr<caspar::core::image_mixer> create_image_mixer(int channel_id);

    std::shared_ptr<accelerator_device> get_device() const;

  private:
    struct impl;
    std::unique_ptr<impl> impl_;
};

}} // namespace caspar::accelerator
