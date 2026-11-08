#pragma once

#include "audio_mixer.h"
#include <common/audio_ring_buffer.h>
#include <core/frame/draw_frame.h>

#include <atomic>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace caspar { namespace core {

struct audio_matrix_source
{
    int    channel;
    int    layer; // -1 is the whole pre-matrix channel bus.
    int    audio_channel;
    bool   post;
    double gain;
};

std::vector<audio_matrix_source> parse_audio_matrix_sources(const std::wstring& value);

class audio_tap
{
  public:
    audio_tap(int                                         channel,
              int                                         layer,
              bool                                        post,
              const video_format_desc&                    format,
              spl::shared_ptr<caspar::diagnostics::graph> graph,
              int                                         maximum_read_samples = 0);

    const int               channel;
    const int               layer;
    const bool              post;
    const video_format_desc format;
    const int               maximum_read_samples;

    void publish(const draw_frame& frame, const video_format_desc& current_format, int nb_samples);
    const std::vector<int32_t>& read(int nb_samples);
    bool                        format_valid() const { return format_valid_.load(std::memory_order_acquire); }

  private:
    spl::shared_ptr<caspar::diagnostics::graph> graph_;
    audio_mixer                                 mixer_;
    audio_ring_buffer<int32_t>                  buffer_;
    std::vector<int32_t>                        scratch_;
    std::atomic<bool>                           format_valid_{true};
    std::atomic<bool>                           discontinuity_{false};
};

struct audio_matrix_input
{
    audio_matrix_source        source;
    std::shared_ptr<audio_tap> tap;
};

struct audio_matrix_config
{
    // nullopt means legacy mixing; an empty input list means explicit silence.
    std::vector<std::optional<std::vector<audio_matrix_input>>> outputs;
    std::wstring                                                text() const;
};

array<const int32_t> mix_audio_matrix(const array<const int32_t>& legacy,
                                      const video_format_desc&    format,
                                      int                         nb_samples,
                                      const audio_matrix_config&  config,
                                      double                      master_volume);

}} // namespace caspar::core
