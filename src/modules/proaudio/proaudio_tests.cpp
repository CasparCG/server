#include "consumer/proaudio_consumer.cpp"

#if __has_include("producer/proaudio_producer.cpp")
#include "producer/proaudio_producer.cpp"
#define TEST_CAPTURE
#endif

#include <core/frame/frame_visitor.h>
#include <iostream>
#include <stdexcept>

namespace {

void check(bool value, const char* message)
{
    if (!value)
        throw std::runtime_error(message);
}

struct mock_stream
{
    PaStreamCallback* callback;
    void*             user;
    int               channels;
};

PaHostApiInfo     api{1, paALSA, "Test API", 2, 1, 1};
PaDeviceInfo      devices[2]{{2, "First device", 0, 2, 2, 0.001, 0.001, 0.01, 0.01, 48000},
                             {2, "Default device", 0, 2, 2, 0.001, 0.001, 0.01, 0.01, 48000}};
PaStreamInfo      stream_info{1, 0.001, 0.001, 48000};
std::atomic<int>  libraries{0};
std::atomic<bool> lifetime_overlap{false};

caspar::core::const_frame pcm_frame(int samples, int channels)
{
    caspar::core::pixel_format_desc desc(caspar::core::pixel_format::bgra);
    desc.planes.emplace_back(1, 1, 4);
    std::vector<caspar::array<const uint8_t>> image;
    image.emplace_back(std::vector<uint8_t>(4));
    return caspar::core::const_frame(
        nullptr,
        std::move(image),
        caspar::array<const int32_t>(std::vector<int32_t>(static_cast<std::size_t>(samples) * channels, 65536)),
        desc);
}

void tick(PaStream* stream, int samples)
{
    auto*                mock = static_cast<mock_stream*>(stream);
    std::vector<int16_t> output(static_cast<std::size_t>(samples) * mock->channels);
    mock->callback(nullptr, output.data(), samples, nullptr, 0, mock->user);
}

#ifdef TEST_CAPTURE
class test_factory final : public caspar::core::frame_factory
{
  public:
    caspar::core::mutable_frame create_frame(const void* tag, const caspar::core::pixel_format_desc& desc) override
    {
        std::vector<caspar::array<uint8_t>> image;
        for (const auto& plane : desc.planes)
            image.emplace_back(std::vector<uint8_t>(plane.size));
        return caspar::core::mutable_frame(tag, std::move(image), {}, desc);
    }
    caspar::core::mutable_frame
    create_frame(const void* tag, const caspar::core::pixel_format_desc& desc, caspar::common::bit_depth) override
    {
        return create_frame(tag, desc);
    }
#ifdef WIN32
    caspar::core::const_frame import_d3d_texture(const void*,
                                                 const std::shared_ptr<caspar::accelerator::d3d::d3d_texture2d>&,
                                                 caspar::core::pixel_format,
                                                 caspar::common::bit_depth) override
    {
        throw std::runtime_error("No D3D texture needed for capture tests");
    }
#endif
};

class capture_pcm final : public caspar::core::frame_visitor
{
  public:
    std::vector<int32_t> audio;
    void                 push(const caspar::core::frame_transform&) override {}
    void                 pop() override {}
    void                 visit(const caspar::core::const_frame& frame) override
    {
        audio.assign(frame.audio_data().begin(), frame.audio_data().end());
    }
};

void capture_tick(PaStream* stream, int samples, int16_t value)
{
    auto*                mock = static_cast<mock_stream*>(stream);
    std::vector<int16_t> input(static_cast<std::size_t>(samples) * mock->channels, value);
    mock->callback(input.data(), nullptr, samples, nullptr, 0, mock->user);
}
#endif

} // namespace

extern "C" {
PaError Pa_Initialize()
{
    if (libraries.fetch_add(1) != 0)
        lifetime_overlap = true;
    return paNoError;
}
PaError Pa_Terminate()
{
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
    if (libraries.fetch_sub(1) != 1)
        lifetime_overlap = true;
    return paNoError;
}
const char*          Pa_GetErrorText(PaError) { return "mock PortAudio error"; }
PaHostApiIndex       Pa_GetHostApiCount() { return 1; }
const PaHostApiInfo* Pa_GetHostApiInfo(PaHostApiIndex index) { return index == 0 ? &api : nullptr; }
PaDeviceIndex        Pa_GetDeviceCount() { return 2; }
const PaDeviceInfo*  Pa_GetDeviceInfo(PaDeviceIndex index)
{
    return index >= 0 && index < 2 ? &devices[index] : nullptr;
}
PaDeviceIndex Pa_GetDefaultInputDevice() { return 1; }
PaDeviceIndex Pa_GetDefaultOutputDevice() { return 1; }
PaError       Pa_OpenStream(PaStream**                stream,
                            const PaStreamParameters* input,
                            const PaStreamParameters* output,
                            double,
                            unsigned long,
                            PaStreamFlags,
                            PaStreamCallback* callback,
                            void*             user)
{
    *stream = new mock_stream{callback, user, (output ? output : input)->channelCount};
    return paNoError;
}
PaError Pa_StartStream(PaStream*) { return paNoError; }
PaError Pa_StopStream(PaStream*) { return paNoError; }
PaError Pa_CloseStream(PaStream* stream)
{
    delete static_cast<mock_stream*>(stream);
    return paNoError;
}
const PaStreamInfo* Pa_GetStreamInfo(PaStream*) { return &stream_info; }
}

int main()
{
    using namespace caspar;
    using namespace caspar::proaudio;
    try {
        check(parse_channel_map("", 2) == std::vector<int>({1, 2}), "identity device map");
        check(parse_channel_map("1,1", 2) == std::vector<int>({1, 1}), "capture fan-out must remain valid");
        for (const auto& value : {"1,,2", "1,", ",1", " ", "1,x", "1,2junk"}) {
            bool rejected = false;
            try {
                parse_channel_map(value, 2);
            } catch (const std::invalid_argument&) {
                rejected = true;
            }
            check(rejected, "malformed device map accepted");
        }
        check(find_device(device_direction::output, "", "Test API") == 1, "host API default ignored");
        check(find_device(device_direction::input, "", "Test API") == 1, "host API input default ignored");
        api.defaultOutputDevice = paNoDevice;
        bool rejected           = false;
        try {
            find_device(device_direction::output, "", "Test API");
        } catch (const invalid_operation&) {
            rejected = true;
        }
        check(rejected, "missing API default accepted");
        api.defaultOutputDevice = 1;
        rejected                = false;
        try {
            find_device(device_direction::output, " ", "");
        } catch (const invalid_operation&) {
            rejected = true;
        }
        check(rejected, "empty normalized device name accepted");

        auto format           = core::video_format_repository().find(L"1080p2500");
        format.audio_channels = 8;
        const core::channel_info channel(1, common::bit_depth::bit8, core::color_space::bt709);
        {
            proaudio_consumer consumer("", "", "", timespan{"0"}, 512, 10);
            consumer.initialize(format, channel, 0);
            consumer.executor_.invoke([] {});
            check(!consumer.failed_ && consumer.channel_map_.size() == 2, "stereo device default failed");
            const int samples = format.audio_cadence.front();
            for (int frame = 0; frame < 1000; ++frame) {
                auto ready = consumer.send(core::video_field::progressive, pcm_frame(samples, 8));
                consumer.executor_.invoke([] {});
                check(consumer.playback_deadline_ == static_cast<uint64_t>(frame + 1) * samples,
                      "callback rounding slowed the output clock");
                while (consumer.frames_played_ < consumer.playback_deadline_)
                    tick(consumer.stream_, 512);
                check(ready.get(), "mock consumer send failed");
            }
            tick(consumer.stream_, 8192);
            auto ready = consumer.send(core::video_field::progressive, pcm_frame(samples, 8));
            consumer.executor_.invoke([] {});
            check(consumer.playback_deadline_ > consumer.frames_played_, "underrun deadline did not recover");
            while (consumer.frames_played_ < consumer.playback_deadline_)
                tick(consumer.stream_, 512);
            check(ready.get(), "underrun recovery send failed");
            std::thread printing([&] {
                for (int i = 0; i < 10000; ++i)
                    check(!consumer.print().empty(), "empty consumer print state");
            });
            for (int i = 0; i < 10; ++i)
                consumer.initialize(format, channel, 0);
            consumer.executor_.invoke([] {});
            printing.join();
            check(consumer.playback_deadline_ == 0 && consumer.frames_played_ == 0,
                  "reinitialization did not reset clock");
        }
        {
            proaudio_consumer consumer("", "", "1,1", timespan{"0"}, 512, 10);
            consumer.initialize(format, channel, 0);
            consumer.executor_.invoke([] {});
            check(consumer.failed_, "duplicate physical output map accepted");
            format.audio_cadence.clear();
            consumer.initialize(format, channel, 0);
            consumer.executor_.invoke([] {});
            check(consumer.failed_, "empty output cadence accepted");
        }
        std::vector<std::thread> threads;
        for (int i = 0; i < 8; ++i)
            threads.emplace_back([] {
                for (int n = 0; n < 100; ++n) {
                    auto library = pa_library::acquire();
                    std::this_thread::yield();
                }
            });
        for (auto& thread : threads)
            thread.join();
        check(!lifetime_overlap && libraries == 0, "PortAudio initialization overlapped teardown");
#ifdef TEST_CAPTURE
        format.audio_cadence = {1920};
        {
            auto              factory = spl::make_shared<test_factory>();
            proaudio_producer producer(factory, format, "", "", "", 512, 10);
            check(producer.channel_map_.size() == 2, "stereo capture identity map failed");
            capture_tick(producer.stream_, 512, 123);
            producer.first_frame(core::video_field::progressive);
            check(producer.stale_capture_, "zero-sample background preview consumed freshness reset");
            capture_pcm captured;
            producer.receive_impl(core::video_field::progressive, 16).accept(captured);
            check(std::all_of(captured.audio.begin(), captured.audio.end(), [](int32_t v) { return v == 0; }),
                  "initial playback leaked queued capture");
            capture_tick(producer.stream_, 16, -123);
            producer.receive_impl(core::video_field::progressive, 16).accept(captured);
            for (std::size_t n = 0; n < captured.audio.size(); ++n)
                check(captured.audio[n] == (n % 8 < 2 ? -123 * 65536 : 0),
                      "capture padding or signed conversion failed");
            producer.last_frame(core::video_field::progressive);
            for (int n = 0; n < 100; ++n)
                capture_tick(producer.stream_, 512, 999);
            producer.receive_impl(core::video_field::progressive, 16).accept(captured);
            check(std::all_of(captured.audio.begin(), captured.audio.end(), [](int32_t v) { return v == 0; }),
                  "resume leaked stale paused capture");
            capture_tick(producer.stream_, 16, 321);
            producer.receive_impl(core::video_field::progressive, 16).accept(captured);
            check(captured.audio[0] == 321 * 65536, "capture did not recover after resume");
        }
#endif
        std::cout << "Proaudio mocked device, lifecycle and playback clock tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
