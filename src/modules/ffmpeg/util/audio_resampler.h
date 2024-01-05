#include <common/array.h>
#include <memory>

extern "C" {
#include <libavutil/samplefmt.h>
}

struct SwrContext;

namespace caspar::ffmpeg {

        class AudioResampler
        {
            std::shared_ptr<SwrContext> ctx;

        public:
            AudioResampler(int64_t sample_rate, AVSampleFormat in_sample_fmt);

            AudioResampler(const AudioResampler&)            = delete;
            AudioResampler& operator=(const AudioResampler&) = delete;

            caspar::array<int32_t> convert(int frames, const void** src);
        };

    }; // namespace caspar::ffmpeg