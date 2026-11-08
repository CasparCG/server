#include <common/diagnostics/graph.h>
#include <common/except.h>
#include <core/consumer/frame_consumer.h>
#include <core/consumer/output.h>
#include <core/frame/frame.h>
#include <core/frame/frame_transform.h>
#include <core/frame/pixel_format.h>
#include <core/mixer/audio/audio_matrix.h>
#include <core/mixer/image/image_mixer.h>
#include <core/producer/cg_proxy.h>
#include <core/producer/frame_producer.h>
#include <core/producer/frame_producer_registry.h>
#include <core/producer/route/route_producer.h>
#include <core/producer/stage.h>
#include <core/video_channel.h>

#include <array>
#include <chrono>
#include <iostream>
#include <thread>

namespace {

void check(bool condition, const char* message)
{
    if (!condition) {
        throw std::runtime_error(message);
    }
}

caspar::core::draw_frame frame(const void* tag, int samples, int channels, int base, bool sequence = false)
{
    std::vector<int32_t> pcm(static_cast<std::size_t>(samples) * channels);
    for (int sample = 0; sample < samples; ++sample) {
        for (int channel = 0; channel < channels; ++channel) {
            pcm[static_cast<std::size_t>(sample) * channels + channel] =
                base + channel + (sequence ? sample * channels : 0);
        }
    }
    caspar::core::pixel_format_desc desc(caspar::core::pixel_format::bgra);
    desc.planes.emplace_back(1, 1, 4);
    std::vector<caspar::array<const uint8_t>> image;
    image.emplace_back(std::vector<uint8_t>(4));
    return caspar::core::draw_frame(
        caspar::core::const_frame(tag, std::move(image), caspar::array<const int32_t>(std::move(pcm)), desc));
}

class test_image_mixer final : public caspar::core::image_mixer
{
  public:
    explicit test_image_mixer(std::atomic<int>& renders)
        : renders_(renders)
    {
    }
    void                      push(const caspar::core::frame_transform&) override {}
    void                      visit(const caspar::core::const_frame&) override {}
    void                      pop() override {}
    void                      update_aspect_ratio(double) override {}
    caspar::common::bit_depth depth() const override { return caspar::common::bit_depth::bit8; }
    std::future<std::tuple<caspar::array<const uint8_t>, std::shared_ptr<caspar::core::texture>>>
    render(const caspar::core::video_format_desc& format, bool) override
    {
        ++renders_;
        return caspar::make_ready_future(std::make_tuple(
            caspar::array<const uint8_t>(std::vector<uint8_t>(format.size)), std::shared_ptr<caspar::core::texture>{}));
    }
    caspar::core::mutable_frame create_frame(const void* tag, const caspar::core::pixel_format_desc& desc) override
    {
        std::vector<caspar::array<uint8_t>> image;
        for (const auto& plane : desc.planes) {
            image.emplace_back(std::vector<uint8_t>(plane.size));
        }
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
        throw std::runtime_error("D3D import is not needed for PCM tests");
    }
#endif
  private:
    std::atomic<int>& renders_;
};

class test_producer final : public caspar::core::frame_producer
{
  public:
    test_producer(int channels, int base)
        : channels_(channels)
        , base_(base)
    {
    }
    caspar::core::draw_frame receive_impl(caspar::core::video_field, int samples) override
    {
        const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
        while (stalled.load()) {
            entered.store(true);
            if (std::chrono::steady_clock::now() >= deadline) {
                stalled.store(false);
                throw std::runtime_error("stalled test producer exceeded its five-second deadline");
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        return frame(this, samples, channels_, base_);
    }
    std::atomic<bool>            stalled{false}, entered{false};
    caspar::core::monitor::state state() const override { return {}; }
    std::wstring                 name() const override { return L"pcm-test"; }
    std::wstring                 print() const override { return name(); }
    bool                         is_ready() override { return true; }

  private:
    int channels_;
    int base_;
};

class test_consumer final : public caspar::core::frame_consumer
{
  public:
    void initialize(const caspar::core::video_format_desc&, const caspar::core::channel_info&, int) override
    {
        ++initializations;
    }
    std::future<bool> send(caspar::core::video_field, caspar::core::const_frame frame) override
    {
        second.store(frame.audio_data().data()[1]);
        first.store(frame.audio_data().data()[0]);
        ++frames;
        return caspar::make_ready_future(true);
    }
    caspar::core::monitor::state state() const override { return {}; }
    std::wstring                 name() const override { return L"pcm-test"; }
    std::wstring                 print() const override { return name(); }
    int                          index() const override { return 100; }
    std::atomic<int>             initializations{0};
    std::atomic<int>             frames{0};
    std::atomic<int32_t>         first{0};
    std::atomic<int32_t>         second{0};
};

void channel_integration()
{
    using namespace caspar;
    using namespace caspar::core;
    video_format_desc format(video_format::custom, 1, 2, 2, 2, 2, 50000, 1000, L"PCMTEST", {960});
    format.audio_channels = 8;
    std::atomic<int> source_renders{0}, destination_renders{0};
    auto             source = spl::make_shared<video_channel>(
        1, format, color_space::bt709, std::make_unique<test_image_mixer>(source_renders), [](monitor::state) {});
    auto destination = spl::make_shared<video_channel>(
        2, format, color_space::bt709, std::make_unique<test_image_mixer>(destination_renders), [](monitor::state) {});
    std::vector<spl::shared_ptr<video_channel>> channels{source, destination};
    auto                                        input = spl::make_shared<test_producer>(8, 100);
    auto                                        local = spl::make_shared<test_producer>(8, 1000);
    source->stage()->load(10, input).get();
    source->stage()->play(10).get();
    destination->stage()->load(20, local).get();
    destination->stage()->play(20).get();
    auto consumer = spl::make_shared<test_consumer>();
    destination->output().add(consumer);
    const auto await = [&](int32_t value) {
        const auto end = std::chrono::steady_clock::now() + std::chrono::seconds(3);
        while (consumer->first.load() != value && std::chrono::steady_clock::now() < end) {
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
        check(consumer->first.load() == value, "threaded channel matrix output failed");
    };
    await(1000);
    destination->configure_audio_matrix(0, std::vector<audio_matrix_source>{{1, 10, 4, true, 0.5}}, channels);
    await(52);
    check(consumer->second.load() == 1001, "threaded matrix changed untouched output");
    const auto previous = destination->audio_matrix_state();
    for (const auto& invalid : std::vector<audio_matrix_source>{
             {99, 10, 0, true, 1}, {1, 10, 8, true, 1}, {1, 10, 0, true, std::numeric_limits<double>::infinity()}}) {
        bool rejected = false;
        try {
            destination->configure_audio_matrix(0, std::vector<audio_matrix_source>{invalid}, channels);
        } catch (const user_error&) {
            rejected = true;
        }
        check(rejected && destination->audio_matrix_state() == previous, "invalid update changed live matrix");
    }
    const auto producer = destination->stage()->foreground(20).get();
    destination->configure_audio_matrix(0, std::vector<audio_matrix_source>{{1, 10, 5, false, 1}}, channels);
    await(105);
    source->stage()->stop(10).get();
    await(0);
    source->stage()->load(10, input).get();
    source->stage()->play(10).get();
    await(105);
    destination->configure_audio_matrix(0, std::vector<audio_matrix_source>{{1, 999, 0, true, 1}}, channels);
    await(0);
    destination->configure_audio_matrix(0, std::vector<audio_matrix_source>{}, channels);
    await(0);
    check(destination->audio_matrix_state() == L"0=SILENCE", "silence update did not replace the active mapping");
    destination->clear_audio_matrix();
    await(1000);
    check(destination->stage()->foreground(20).get().get() == producer.get(), "matrix recreated producer");
    check(consumer->initializations.load() == 1 && consumer->frames.load() > 3, "matrix restarted consumer/video");
    check(source_renders.load() == 0 && destination_renders.load() > 0, "audio-only tap rendered source video");
    check(destination->audio_matrix_state() == L"DEFAULT", "matrix default reset failed");

    frame_producer_dependencies dependencies(destination->frame_factory(),
                                             channels,
                                             video_format_repository(),
                                             format,
                                             spl::make_shared<frame_producer_registry>(),
                                             spl::make_shared<cg_producer_registry>());
    auto route = create_route_producer(dependencies, {L"route://1-10", L"AUDIO_MAP", L"0=4,1=5"});
    destination->stage()->stop(20).get();
    destination->stage()->load(30, route).get();
    destination->stage()->play(30).get();
    await(104);
    check(consumer->second.load() == 105, "native route creation mapping failed");
    destination->stage()->call(30, {L"AUDIO_MAP", L"0=6,1=7"}).get();
    await(106);
    check(consumer->second.load() == 107, "native route live mapping failed");
    bool rejected = false;
    try {
        destination->stage()->call(30, {L"AUDIO_MAP", L"0=0,0=1"}).get();
    } catch (const user_error&) {
        rejected = true;
    }
    check(rejected && destination->stage()->call(30, {L"AUDIO_MAP"}).get() == L"0=6,1=7",
          "invalid native route update changed mapping");
    destination->stage()->call(30, {L"AUDIO_MAP", L"DEFAULT"}).get();
    await(100);
    check(destination->stage()->foreground(30).get().get() == route.get(), "route CALL recreated producer");
    check(consumer->initializations.load() == 1, "route CALL recreated consumer");

    destination->stage()->call(30, {L"AUDIO_MAP", L"0=4,1=5"}).get();
    await(104);
    input->stalled.store(true);
    const auto stall_deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
    while (!input->entered.load() && std::chrono::steady_clock::now() < stall_deadline)
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    if (!input->entered.load()) {
        input->stalled.store(false);
        throw std::runtime_error("test source did not enter its stall within five seconds");
    }
    destination->stage()->call(30, {L"AUDIO_MAP", L"DEFAULT"}).get();
    std::this_thread::sleep_for(std::chrono::milliseconds(150));
    const bool muted_repeat = consumer->first.load() == 0 && consumer->second.load() == 0;
    input->stalled.store(false);
    check(muted_repeat, "DEFAULT replayed previously buffered mapped audio while source was late");
    destination->stage()->call(30, {L"AUDIO_MAP", L"0=4,1=5"}).get();
    auto narrow           = format;
    narrow.audio_channels = 2;
    source->stage()->stop(10).get();
    source->stage()->video_format_desc(narrow).get();
    source->stage()->load(10, spl::make_shared<test_producer>(2, 2000)).get();
    source->stage()->play(10).get();
    await(0);
    destination->stage()->call(30, {L"AUDIO_MAP", L"0=1,1=0"}).get();
    await(2001);
    check(consumer->second.load() == 2000, "route used stale source PCM stride after format change");
}

} // namespace

int main(int argc, char** argv)
{
    using namespace caspar;
    using namespace caspar::core;
    try {
        check(argc == 2 && (std::string(argv[1]) == "--unit" || std::string(argv[1]) == "--module"),
              "use --unit for PCM logic or --module for headless channel/route integration");
        if (std::string(argv[1]) == "--module") {
            channel_integration();
            std::cout << "Headless audio channel and route integration tests passed\n";
            return 0;
        }
        auto                    graph = spl::make_shared<diagnostics::graph>();
        video_format_repository repository;
        auto                    format                    = repository.find(L"1080p5000");
        format.audio_channels                             = 8;
        const int                                 samples = format.audio_cadence.front();
        int                                       tags[4]{};
        std::array<std::shared_ptr<audio_tap>, 4> taps;
        audio_matrix_config                       matrix;
        matrix.outputs.resize(8);
        for (int route = 0; route < 4; ++route) {
            taps[route] = std::make_shared<audio_tap>(route + 1, 10, true, format, graph);
            taps[route]->publish(frame(&tags[route], samples, 8, (route + 1) * 100), format, samples);
            for (int channel = 0; channel < 2; ++channel) {
                matrix.outputs[route * 2 + channel] =
                    std::vector<audio_matrix_input>{{{route + 1, 10, channel, true, 1.0}, taps[route]}};
            }
        }
        array<const int32_t> legacy(std::vector<int32_t>(static_cast<std::size_t>(samples) * 8, 999));
        auto                 mixed = mix_audio_matrix(legacy, format, samples, matrix, 1.0);
        for (int sample = 0; sample < samples; ++sample) {
            for (int channel = 0; channel < 8; ++channel) {
                check(mixed.data()[sample * 8 + channel] == (channel / 2 + 1) * 100 + channel % 2,
                      "four-source pair isolation failed");
            }
        }

        for (int route = 0; route < 4; ++route) {
            taps[route]->publish(frame(&tags[route], samples, 8, (route + 1) * 100), format, samples);
        }
        matrix.outputs[2]->front().source.audio_channel = 4;
        matrix.outputs[3]->front().source.audio_channel = 5;
        matrix.outputs[6]                               = std::nullopt;
        matrix.outputs[7]                               = std::vector<audio_matrix_input>{};
        mixed                                           = mix_audio_matrix(legacy, format, samples, matrix, 1.0);
        check(mixed.data()[2] == 204 && mixed.data()[3] == 205, "live input selection failed");
        check(mixed.data()[0] == 100 && mixed.data()[4] == 300, "live update contaminated other pairs");
        check(mixed.data()[6] == 999 && mixed.data()[7] == 0, "legacy/silence output policy failed");

        auto            pre    = std::make_shared<audio_tap>(1, 10, false, format, graph);
        auto            post   = std::make_shared<audio_tap>(1, 10, true, format, graph);
        auto            source = frame(&tags[0], samples, 8, 1000);
        frame_transform fader;
        fader.audio_transform.volume           = 0.25;
        fader.audio_transform.immediate_volume = true;
        auto layer                             = draw_frame::push(source, fader);
        pre->publish(layer, format, samples);
        post->publish(layer, format, samples);
        matrix.outputs.assign(8, std::nullopt);
        matrix.outputs[0] =
            std::vector<audio_matrix_input>{{{1, 10, 0, false, 0.5}, pre}, {{1, 10, 0, true, 2.0}, post}};
        matrix.outputs[1] = std::vector<audio_matrix_input>{{{1, 10, 1, false, -1.0}, pre}};
        mixed             = mix_audio_matrix(legacy, format, samples, matrix, 0.5);
        check(mixed.data()[0] == 500 && mixed.data()[1] == -500, "pre/post gains or master volume failed");
        pre->publish(frame(&tags[0], samples, 8, 1 << 29), format, samples);
        matrix.outputs[0] = std::vector<audio_matrix_input>{{{1, 10, 0, false, 16}, pre}};
        matrix.outputs[1] = std::vector<audio_matrix_input>{{{1, 10, 0, false, -16}, pre}};
        mixed             = mix_audio_matrix(legacy, format, samples, matrix, 1);
        check(mixed.data()[0] == std::numeric_limits<int32_t>::max() &&
                  mixed.data()[1] == std::numeric_limits<int32_t>::min(),
              "matrix saturation failed");
        audio_mixer meters(graph);
        meters.update_meter(mixed, 8);
        const auto final_meter = meters.state();
        check(boost::get<int32_t>(final_meter.begin()->second[0]) == std::numeric_limits<int32_t>::max() &&
                  boost::get<int32_t>(final_meter.begin()->second[1]) == std::numeric_limits<int32_t>::max(),
              "final-output clipping peaks were not reported");
        auto source_post = std::make_shared<audio_tap>(1, 10, true, format, graph);
        source_post->publish(draw_frame::push(source), format, samples);
        check(source_post->read(samples)[0] == 1000, "matrix changed shared source audio");

        pre->publish(draw_frame{}, format, samples);
        check(pre->read(samples)[0] == 0, "missing source audio leaked stale PCM");
        check(pre->read(samples)[0] == 0, "transport underflow did not produce silence");
        auto changed           = format;
        changed.audio_channels = 2;
        pre->publish(source, changed, samples);
        check(!pre->format_valid() && pre->read(samples)[0] == 0, "unsafe format change");
        pre->publish(source, format, samples);
        check(pre->format_valid() && pre->read(samples)[0] == 1000, "source recovery failed");

        for (int tick = 0; tick < 10; ++tick) {
            pre->publish(source, format, samples);
        }
        check(pre->read(samples)[0] == 0, "overflow replayed old audio");
        pre->publish(source, format, samples);
        check(pre->read(samples)[0] == 1000, "overflow recovery failed");

        auto fractional           = repository.find(L"1080p5994");
        fractional.audio_channels = 8;
        auto        cadence_tap   = std::make_shared<audio_tap>(1, 10, true, fractional, graph);
        std::size_t total         = 0;
        for (int tick = 0; tick < 6000; ++tick) {
            const int count = fractional.audio_cadence[tick % fractional.audio_cadence.size()];
            cadence_tap->publish(frame(&tags[0], count, 8, 100 + static_cast<int>(total) * 8, true), fractional, count);
            const auto& pcm = cadence_tap->read(count);
            check(pcm.size() == static_cast<std::size_t>(count) * 8, "fractional cadence size changed");
            // The existing mixer inserts one PCM frame of silence at variable-cadence startup.
            const auto start = tick == 0 ? 8 : 0;
            for (std::size_t scalar = start; scalar < pcm.size(); ++scalar) {
                check(pcm[scalar] == 100 + static_cast<int>(total * 8 + scalar) - 8,
                      "fractional cadence PCM dropped or repeated samples");
            }
            total += count;
        }
        check(total == 4804800, "sustained 59.94 sample count incorrect");

        auto interlaced           = repository.find(L"1080i5994");
        interlaced.audio_channels = 8;
        auto field_tap            = std::make_shared<audio_tap>(1, 10, true, interlaced, graph);
        total                     = 0;
        int     source_cursor     = 0;
        int64_t expected_scalar   = -1;
        for (int tick = 0; tick < 3000; ++tick) {
            const int source_count = interlaced.audio_cadence[tick % interlaced.audio_cadence.size()];
            for (int field = 0; field < 2; ++field) {
                field_tap->publish(
                    frame(&tags[0], source_count, 8, 100 + source_cursor * 8, true), interlaced, source_count);
                source_cursor += source_count;
            }
            for (int field = 0; field < 2; ++field) {
                const int   count = fractional.audio_cadence[(tick * 2 + field) % fractional.audio_cadence.size()];
                const auto& pcm   = field_tap->read(count);
                check(pcm.size() == static_cast<std::size_t>(count) * 8, "i-to-p field cadence changed");
                if (tick > 10) {
                    if (expected_scalar < 0)
                        expected_scalar = pcm.front();
                    for (std::size_t scalar = 0; scalar < pcm.size(); ++scalar) {
                        check(pcm[scalar] == expected_scalar++, "i-to-p cadence dropped or repeated PCM");
                    }
                }
                total += count;
            }
        }
        check(total == 4804800, "i-to-p sustained sample count incorrect");

        auto fast           = repository.find(L"1080p6000");
        fast.audio_channels = 8;
        auto slow           = repository.find(L"1080p2500");
        slow.audio_channels = 8;
        audio_tap rate_tap(1, 10, true, fast, graph, slow.audio_cadence.front());
        int       queued    = 0;
        int       published = 0, consumed = 0;
        for (int tick = 0; tick < 2500; ++tick) {
            while (queued < slow.audio_cadence.front()) {
                rate_tap.publish(frame(&tags[0], fast.audio_cadence.front(), 8, 700 + published * 8, true),
                                 fast,
                                 fast.audio_cadence.front());
                queued += fast.audio_cadence.front();
                published += fast.audio_cadence.front();
            }
            const auto& pcm = rate_tap.read(slow.audio_cadence.front());
            for (std::size_t scalar = 0; scalar < pcm.size(); ++scalar)
                check(pcm[scalar] == 700 + consumed * 8 + static_cast<int>(scalar),
                      "60-to-25 tap dropped or repeated PCM");
            queued -= slow.audio_cadence.front();
            consumed += slow.audio_cadence.front();
        }

        matrix.outputs.assign(8, std::nullopt);
        matrix.outputs[0] = std::vector<audio_matrix_input>{{{1, 10, 0, false, 0.12345678901234567}, pre}};
        const auto text   = matrix.text();
        check(parse_audio_matrix_sources(text.substr(text.find(L'=') + 1)).front().gain ==
                  matrix.outputs[0]->front().source.gain,
              "matrix state lost gain precision");

        meters.update_meter(array<const int32_t>(std::vector<int32_t>(8, 0)), 8);
        frame(&tags[0], samples, 8, std::numeric_limits<int32_t>::max() - 8).accept(meters);
        meters(format, samples, false);
        const auto meter_state = meters.state();
        for (const auto& value : meter_state.begin()->second)
            check(boost::get<int32_t>(value) == 0, "legacy mix updated meters before matrix overrides");

        const auto parsed = parse_audio_matrix_sources(L"1:10:0:PRE:0.5,2:*:7:POST:-1");
        check(parsed.size() == 2 && parsed[1].layer == -1 && parsed[1].gain == -1, "matrix syntax parsing failed");
        for (const auto& invalid :
             {L"1:10:0:POST:nan", L"1:10:0:WRONG:1", L"1:10:0:PRE:17", L"1:10:0:PRE:1,", L"1:10:0:PRE:1junk"}) {
            bool rejected = false;
            try {
                parse_audio_matrix_sources(invalid);
            } catch (const user_error&) {
                rejected = true;
            }
            check(rejected, "invalid matrix input accepted");
        }

        audio_ring_buffer<int32_t> ring(7, 2);
        std::thread                writer([&] {
            for (int value = 1; value <= 10000; ++value) {
                const int32_t pcm[2]{value, -value};
                while (ring.push(pcm, 1) != 1) {
                    std::this_thread::yield();
                }
            }
        });
        for (int value = 1; value <= 10000; ++value) {
            int32_t pcm[2]{};
            while (ring.pop(pcm, 1) == 0) {
                std::this_thread::yield();
            }
            check(pcm[0] == value && pcm[1] == -value, "concurrent FIFO corruption");
        }
        writer.join();
        std::cout << "Audio matrix PCM, tap, cadence and FIFO tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
