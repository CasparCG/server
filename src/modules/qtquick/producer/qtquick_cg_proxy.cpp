#include "qtquick_cg_proxy.h"

#include <future>

namespace caspar { namespace qtquick {

qtquick_cg_proxy::qtquick_cg_proxy(spl::shared_ptr<core::frame_producer> producer)
    : producer_(producer)
{
}

void qtquick_cg_proxy::add(int                 layer,
                           const std::wstring& template_name,
                           bool                play_on_load,
                           const std::wstring& start_from_label,
                           const std::wstring& data)
{
    update(layer, data);

    if (play_on_load)
        play(layer);
}

void qtquick_cg_proxy::remove(int layer) { producer_->call({L"_remove"}); }

void qtquick_cg_proxy::play(int layer) { producer_->call({L"_play"}); }

void qtquick_cg_proxy::stop(int layer) { producer_->call({L"_stop"}); }

void qtquick_cg_proxy::next(int layer) { producer_->call({L"_next"}); }

void qtquick_cg_proxy::update(int layer, const std::wstring& data) { producer_->call({L"_update", data}); }

std::wstring qtquick_cg_proxy::invoke(int layer, const std::wstring& label) { return producer_->call({label}).get(); }

}} // namespace caspar::qtquick
