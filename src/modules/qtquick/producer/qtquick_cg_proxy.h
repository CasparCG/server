#pragma once

#include <common/memory.h>

#include <core/producer/cg_proxy.h>

namespace caspar { namespace qtquick {

class qtquick_cg_proxy : public core::cg_proxy
{
  public:
    qtquick_cg_proxy(spl::shared_ptr<core::frame_producer> producer);

    void         add(int                 layer,
                     const std::wstring& template_name,
                     bool                play_on_load,
                     const std::wstring& start_from_label,
                     const std::wstring& data) override;
    void         remove(int layer) override;
    void         play(int layer) override;
    void         stop(int layer, unsigned int mix_out_duration) override;
    void         next(int layer) override;
    void         update(int layer, const std::wstring& data) override;
    std::wstring invoke(int layer, const std::wstring& label) override;

  private:
    spl::shared_ptr<core::frame_producer> producer_;
};

}} // namespace caspar::qtquick
