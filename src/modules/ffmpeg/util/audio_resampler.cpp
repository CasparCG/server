#include "audio_resampler.h"
#include "av_assert.h"

extern "C" {
#include <libavutil/samplefmt.h>
#include <libswresample/swresample.h>
}

namespace caspar::ffmpeg {

        AudioResampler::AudioResampler(int64_t sample_rate, AVSampleFormat in_sample_fmt)
                : ctx(std::shared_ptr<SwrContext>(swr_alloc_set_opts(nullptr,
                                                                     AV_CH_LAYOUT_7POINT1,
                                                                     AV_SAMPLE_FMT_S32,
                                                                     sample_rate,
                                                                     AV_CH_LAYOUT_7POINT1,
                                                                     in_sample_fmt,
                                                                     sample_rate,
                                                                     0,
                                                                     nullptr),
                                                  [](SwrContext* ptr) { swr_free(&ptr); }))
        {
            if (!ctx)
                FF_RET(AVERROR(ENOMEM), "swr_alloc_set_opts");

            FF_RET(swr_init(ctx.get()), "swr_init");
        }

        caspar::array<int32_t> AudioResampler::convert(int frames, const void** src)
        {
            auto result = caspar::array<int32_t>(frames * 8 * sizeof(int32_t));
            auto ptr    = result.data();
            auto ret    = swr_convert(ctx.get(), (uint8_t**)&ptr, frames, reinterpret_cast<const uint8_t**>(src), frames);

            return result;
        }

    }; // namespace caspar::ffmpeg