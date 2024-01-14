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
                             std::shared_ptr<core::stage_base>    s,
                             const std::wstring&                  lifecycle_key)
        : raw_channel(std::move(c))
        , stage(std::move(s))
        , lifecycle_key_(lifecycle_key)
        , tmp_executor_(new executor(L"Tmp Stage " + std::to_wstring(raw_channel->index())))
    {
    }
    const std::shared_ptr<core::video_channel> raw_channel;
    const std::shared_ptr<core::stage_base>    stage;
    const std::wstring                         lifecycle_key_;
    const std::shared_ptr<executor>            tmp_executor_;
};

struct command_context_simple
{
    const std::wstring              client_address;
    const int                       channel_index;
    const int                       layer_id;
    const std::vector<std::wstring> parameters;

    int layer_index(int default_ = 0) const { return layer_id == -1 ? default_ : layer_id; }

    command_context_simple(const std::wstring&              client_address,
                           int                              channel_index,
                           int                              layer_id,
                           const std::vector<std::wstring>& parameters)
        : client_address(client_address)
        , channel_index(channel_index)
        , layer_id(layer_id)
        , parameters(parameters)
    {
    }
};

typedef std::function<std::future<std::wstring>(const command_context_simple&                        args,
                                                const spl::shared_ptr<std::vector<channel_context>>& channels)>
    amcp_command_func;

}}} // namespace caspar::protocol::amcp
