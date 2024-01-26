#pragma once

#include <common/executor.h>

#include <core/producer/stage.h>
#include <core/video_channel.h>
#include <utility>

namespace caspar { namespace protocol { namespace amcp {

class channel_context
{
  public:
    explicit channel_context() {}
    explicit channel_context(std::shared_ptr<core::video_channel> c,
                             std::shared_ptr<core::stage>         s,
                             const std::wstring&                  lifecycle_key)
        : raw_channel(std::move(c))
        , stage(std::move(s))
        , tmp_executor_(new executor(L"Tmp Stage " + std::to_wstring(raw_channel->index())))
    {
    }
    const std::shared_ptr<core::video_channel> raw_channel;
    const std::shared_ptr<core::stage>         stage;
    // Hack: temporary executor to allow nodejs to queue tasks to the stage along with a custom response handler
    const std::shared_ptr<executor> tmp_executor_;
};

}}} // namespace caspar::protocol::amcp
