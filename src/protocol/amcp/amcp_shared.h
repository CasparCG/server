#pragma once

#include "../util/lock_container.h"
#include <core/video_channel.h>

namespace caspar { namespace protocol { namespace amcp {

class channel_context
{
  public:
    explicit channel_context() {}
    explicit channel_context(const std::shared_ptr<core::video_channel>& c, const std::wstring& lifecycle_key)
        : channel(c)
        , lock(std::make_shared<caspar::IO::lock_container>(lifecycle_key))
    {
    }
    std::shared_ptr<core::video_channel>        channel;
    std::shared_ptr<caspar::IO::lock_container> lock;
};

}}} // namespace caspar::protocol::amcp