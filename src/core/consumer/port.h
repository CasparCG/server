#pragma once

#include "../monitor/monitor.h"

#include <common/memory.h>

#include <future>

namespace caspar { namespace core {

class port
{
    port(const port&);
    port& operator=(const port&);

  public:
    port(int index, int channel_index, spl::shared_ptr<class frame_consumer> consumer);
    port(port&& other);
    ~port();

    port& operator=(port&& other);

    std::future<bool> send(class const_frame frame);

    const monitor::state& state() const;

    void                                  change_channel_format(const struct video_format_desc& format_desc);
    std::wstring                          print() const;
    int                                   buffer_depth() const;
    bool                                  has_synchronization_clock() const;
    spl::shared_ptr<const frame_consumer> consumer() const;

  private:
    struct impl;
    std::unique_ptr<impl> impl_;
};

}} // namespace caspar::core
