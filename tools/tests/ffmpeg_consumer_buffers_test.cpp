#include <modules/ffmpeg/consumer/buffer_config.h>

#include <tbb/concurrent_queue.h>

#include <cstdint>
#include <future>
#include <iostream>
#include <stdexcept>
#include <tuple>

namespace {

void require(bool condition, const char* message)
{
    if (!condition) {
        throw std::runtime_error(message);
    }
}

template <typename F>
void require_user_error(F&& action)
{
    try {
        action();
    } catch (const caspar::user_error&) {
        return;
    }
    throw std::runtime_error("Expected a user error for invalid buffer configuration");
}

void configuration()
{
    using caspar::ffmpeg::buffer_config;
    const buffer_config stream(true);
    const buffer_config file(false);
    require(stream.frames == 6 && stream.packets == 16, "STREAM defaults");
    require(file.frames == 64 && file.packets == 128, "FILE defaults");

    {
        buffer_config             frame_only(true);
        std::vector<std::wstring> frame_args{L"-frame-buffer", L"12"};
        frame_only.consume_options(frame_args);
        require(frame_only.frames == 12 && frame_only.packets == stream.packets,
                "Frame override must leave packet default unchanged");
        buffer_config             packet_only(true);
        std::vector<std::wstring> packet_args{L"-packet-buffer", L"24"};
        packet_only.consume_options(packet_args);
        require(packet_only.frames == stream.frames && packet_only.packets == 24,
                "Packet override must leave frame default unchanged");
    }

    buffer_config             config(true);
    std::vector<std::wstring> args{
        L"-format", L"mpegts", L"-FRAME-BUFFER", L"8", L"-codec:a", L"libopus", L"-packet-buffer", L"32"};
    config.consume_options(args);
    require(config.frames == 8 && config.packets == 32, "Independent AMCP overrides");
    require(args == std::vector<std::wstring>{L"-format", L"mpegts", L"-codec:a", L"libopus"},
            "Consumer options must not reach FFmpeg; preserve other options");

    for (const auto& option : {L"-frame-buffer", L"-packet-buffer"}) {
        for (const auto& value : {L"0", L"-1", L"1.5", L"", L"abc", L"999999999999999999999"}) {
            std::vector<std::wstring> invalid{option, value};
            require_user_error([&] { config.consume_options(invalid); });
        }
        std::vector<std::wstring> missing{option};
        require_user_error([&] { config.consume_options(missing); });
    }
    std::vector<std::wstring> too_many_frames{L"-frame-buffer", L"65"};
    require_user_error([&] { config.consume_options(too_many_frames); });
    std::vector<std::wstring> too_many_packets{L"-packet-buffer", L"1025"};
    require_user_error([&] { config.consume_options(too_many_packets); });

    boost::property_tree::wptree tree;
    config = buffer_config(true);
    config.read_config(tree);
    require(config.frames == 6 && config.packets == 16, "Absent XML settings retain STREAM defaults");
    config = buffer_config(false);
    config.read_config(tree);
    require(config.frames == 64 && config.packets == 128, "Absent XML settings retain FILE defaults");
    for (const auto& option : {L"-frame-buffer", L"-packet-buffer"}) {
        std::vector<std::wstring> file_args{option, L"8"};
        require_user_error([&] { config.consume_options(file_args); });
    }
    for (const auto& key : {L"frame-buffer", L"packet-buffer"}) {
        boost::property_tree::wptree non_realtime;
        non_realtime.put(key, L"8");
        require_user_error([&] { config.read_config(non_realtime); });
    }
    std::vector<std::wstring> file_args{L"-format", L"mpegts"};
    config.consume_options(file_args);
    require(config.frames == 64 && config.packets == 128 &&
                file_args == std::vector<std::wstring>{L"-format", L"mpegts"},
            "FILE capacities and FFmpeg options remain unchanged");
    config = buffer_config(true);
    tree.put(L"frame-buffer", L"1");
    tree.put(L"packet-buffer", L"1024");
    config.read_config(tree);
    require(config.frames == 1 && config.packets == 1024, "XML limits");
    tree.put(L"frame-buffer", L"64");
    config.read_config(tree);
    require(config.frames == 64, "Maximum input capacity");
    for (const auto& key : {L"frame-buffer", L"packet-buffer"}) {
        for (const auto& value : {L"0", L"-1", L"1.5", L"", L"abc", L"999999999999999999999"}) {
            boost::property_tree::wptree invalid;
            invalid.put(key, value);
            require_user_error([&] { config.read_config(invalid); });
        }
    }
    tree.put(L"packet-buffer", L"1025");
    require_user_error([&] { config.read_config(tree); });
    tree.put(L"packet-buffer", L"16");
    tree.put(L"frame-buffer", L"65");
    require_user_error([&] { config.read_config(tree); });

    config = buffer_config(true);
    std::vector<std::wstring> legacy{L"-frame-buffer", L"1", L"-packet-buffer", L"1"};
    config.consume_options(legacy);
    require(config.frames == 1 && config.packets == 1 && legacy.empty(), "Legacy STREAM opt-in");
}

void transient_and_overload()
{
    // Exercise the same TBB queue contract used for admission, without codecs or hardware.
    using input = std::tuple<int, std::int64_t, std::int64_t>;
    const caspar::ffmpeg::buffer_config  config(true);
    tbb::concurrent_bounded_queue<input> frames;
    frames.set_capacity(config.frames);

    // 48 kHz at 60000/1001 inputs/sec: fractional audio cadence, not 800 fixed samples.
    auto audio_pts = [](std::int64_t n) { return n * 48000 * 1001 / 60000; };
    for (int batch = 0; batch < 1000; ++batch) {
        const auto first = batch * config.frames;
        for (int n = first; n < first + config.frames; ++n) {
            require(frames.try_push({n, n, audio_pts(n)}), "Six-frame transient must not reject input");
        }
        for (int n = first; n < first + config.frames; ++n) {
            input value;
            require(frames.try_pop(value), "Drain accepted input");
            require(value == input{n, n, audio_pts(n)}, "Preserve frame order and fractional audio timestamps");
        }
        require(frames.empty(), "Capacity must not impose a permanent backlog");
    }

    for (int n = 0; n < config.frames; ++n) {
        require(frames.try_push({n, n, audio_pts(n)}), "Fill input queue");
    }
    std::uint64_t dropped_samples = 0;
    for (int n = config.frames; n < config.frames + 100; ++n) {
        require(!frames.try_push({n, n, audio_pts(n)}), "Sustained overload must remain bounded");
        dropped_samples += audio_pts(n + 1) - audio_pts(n);
    }
    require(frames.size() == config.frames, "Input capacity is a hard limit");
    require(dropped_samples == static_cast<std::uint64_t>(audio_pts(config.frames + 100) - audio_pts(config.frames)),
            "Rejected sample accounting must follow fractional cadence");
    input value;
    while (frames.try_pop(value)) {
    }
    const int resumed = config.frames + 100;
    require(frames.try_push({resumed, resumed, audio_pts(resumed)}), "Admission resumes after stall");
    require(frames.try_pop(value) && std::get<2>(value) == audio_pts(resumed),
            "Source timeline must not collapse across rejected frames");
}

void packet_backpressure()
{
    const caspar::ffmpeg::buffer_config config(true);
    tbb::concurrent_bounded_queue<int>  packets;
    packets.set_capacity(config.packets);
    for (int n = 0; n < config.packets; ++n) {
        require(packets.try_push(n), "Packet burst must fit configured queue");
    }
    require(!packets.try_push(config.packets), "Packet queue capacity is bounded");

    std::promise<void> started;
    auto               producer = std::async(std::launch::async, [&] {
        started.set_value();
        packets.push(config.packets);
    });
    started.get_future().wait();
    // A blocked encoder can resume when output consumes one packet.
    int packet = -1;
    packets.pop(packet);
    require(packet == 0, "Packet FIFO order");
    producer.get();
    for (int n = 1; n <= config.packets; ++n) {
        packets.pop(packet);
        require(packet == n, "No encoded packet loss on backpressure");
    }
    require(packets.empty(), "Drain packet backlog");
}

} // namespace

int main()
{
    try {
        configuration();
        transient_and_overload();
        packet_backpressure();
        std::cout << "FFmpeg consumer buffer configuration and queue-contract tests passed\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
