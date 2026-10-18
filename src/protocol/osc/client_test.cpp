#include "client.h"
#include "oscpack/OscReceivedElements.h"

#include <common/diagnostics/graph.h>
#include <core/frame/frame_transform.h>
#include <core/producer/stage.h>

#include <boost/asio.hpp>

#include <array>
#include <chrono>
#include <iostream>
#include <iterator>
#include <map>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

namespace {

namespace monitor = caspar::core::monitor;
using boost::asio::ip::udp;
using caspar::protocol::osc::client;
using values_t = std::map<std::string, std::vector<float>>;

void require(bool condition, const std::string& message)
{
    if (!condition)
        throw std::runtime_error(message);
}

monitor::state transform()
{
    monitor::state state;
    state["opacity"]          = 1.0;
    state["contrast"]         = 1.0;
    state["brightness"]       = 1.0;
    state["saturation"]       = 1.0;
    state["anchor"]           = {0.0, 0.0};
    state["fill_translation"] = {0.0, 0.0};
    state["fill_scale"]       = {1.0, 1.0};
    state["clip_translation"] = {0.0, 0.0};
    state["clip_scale"]       = {1.0, 1.0};
    state["angle"]            = 0.0;
    state["volume"]           = 1.0;
    return state;
}

std::string layer_path(int channel) { return "/channel/" + std::to_string(channel) + "/stage/layer/10"; }

monitor::state snapshot(int channel, const monitor::state& properties, std::int32_t tick)
{
    monitor::state state;
    state[layer_path(channel)]["transform"] = properties;
    // Producer properties with a nested "transform" must retain their existing cadence.
    state[layer_path(channel)]["foreground"]["transform"]["opacity"] = 0.75;
    state["/test/tick"]                                              = tick;
    return state;
}

values_t receive(udp::socket& socket, std::int32_t tick)
{
    values_t                values;
    const auto              deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
    std::array<char, 65536> buffer;
    while (std::chrono::steady_clock::now() < deadline) {
        boost::system::error_code ec;
        const auto                size = socket.receive(boost::asio::buffer(buffer), 0, ec);
        if (ec == boost::asio::error::would_block || ec == boost::asio::error::try_again) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            continue;
        }
        if (ec)
            throw boost::system::system_error(ec);

        require(size <= 2048, "OSC bundle exceeded 2048 bytes");
        ::osc::ReceivedBundle bundle(::osc::ReceivedPacket(buffer.data(), static_cast<::osc::int32>(size)));
        require(bundle.TimeTag() != 0, "OSC bundle has a zero timestamp");
        bool complete = false;
        for (auto it = bundle.ElementsBegin(); it != bundle.ElementsEnd(); ++it) {
            ::osc::ReceivedMessage message(*it);
            const std::string      address = message.AddressPattern();
            if (address == "/test/tick") {
                require(message.ArgumentCount() == 1, "Tick must have one argument");
                complete = message.ArgumentsBegin()->AsInt32() == tick;
                continue;
            }
            std::vector<float> args;
            for (auto arg = message.ArgumentsBegin(); arg != message.ArgumentsEnd(); ++arg) {
                if (arg->IsFloat())
                    args.push_back(arg->AsFloat());
            }
            if (address.find("/transform/") != std::string::npos)
                require(args.size() == message.ArgumentCount(), "Transform arguments must all be floats");
            values[address] = std::move(args);
        }
        if (complete)
            return values;
    }
    throw std::runtime_error("Timed out waiting for OSC tick " + std::to_string(tick));
}

void require_transform(const values_t& received, int channel, const monitor::state& expected)
{
    const auto  prefix = layer_path(channel) + "/transform/";
    std::size_t count  = 0;
    for (const auto& p : received) {
        if (p.first.starts_with(prefix))
            ++count;
    }
    require(count == static_cast<std::size_t>(std::distance(expected.begin(), expected.end())),
            "Unexpected number of transform properties");
    for (const auto& p : expected) {
        const auto it = received.find(prefix + p.first);
        require(it != received.end(), "Missing property " + p.first);
        std::vector<float> args;
        for (const auto& arg : p.second)
            args.push_back(static_cast<float>(boost::get<double>(arg)));
        require(it->second == args, "Wrong OSC argument values or shape for " + p.first);
    }
}

void test_stage_snapshots()
{
    namespace core = caspar::core;
    core::stage stage(1,
                      caspar::spl::make_shared<caspar::diagnostics::graph>(),
                      core::video_format_repository().find_format(core::video_format::x720p5000));
    stage.pause(10).get();

    auto        context = std::make_shared<boost::asio::io_context>();
    udp::socket socket(*context, udp::endpoint(boost::asio::ip::address_v4::loopback(), 0));
    socket.non_blocking(true);
    client           sender(context);
    auto             token = sender.get_subscription_token(socket.local_endpoint());
    std::uint64_t    frame = 0;
    std::vector<int> backgrounds;
    auto             produce = [&] {
        stage(++frame, backgrounds, [](int, const core::layer_frame&) {});
        monitor::state state;
        state[""]["channel"][1]["stage"] = stage.state();
        state["/test/tick"]              = static_cast<std::int32_t>(frame);
        return state;
    };
    auto send = [&] {
        sender.send(1, produce());
        return receive(socket, static_cast<std::int32_t>(frame));
    };

    require_transform(send(), 1, transform());
    require_transform(send(), 1, {});
    stage
        .apply_transform(
            10,
            [](core::frame_transform value) {
                value.image_transform.opacity = 0.5;
                return value;
            },
            0,
            caspar::tweener())
        .get();
    produce(); // Simulate a channel tick that fails before OSC submission.
    monitor::state change;
    change["opacity"] = 0.5;
    require_transform(send(), 1, change);
    require_transform(send(), 1, {});

    stage
        .apply_transform(
            10,
            [](core::frame_transform value) {
                value.image_transform.enable_geometry_modifiers = true;
                return value;
            },
            0,
            caspar::tweener())
        .get();
    require_transform(send(), 1, {});

    stage
        .apply_transform(
            10,
            [](core::frame_transform value) {
                value.image_transform.angle = 1.0;
                return value;
            },
            4,
            caspar::tweener())
        .get();
    for (int step = 1; step <= 4; ++step) {
        change          = {};
        change["angle"] = static_cast<double>(step) / 4;
        require_transform(send(), 1, change);
    }
    require_transform(send(), 1, {});
}

void run()
{
    test_stage_snapshots();
    auto        context = std::make_shared<boost::asio::io_context>();
    udp::socket first(*context, udp::endpoint(boost::asio::ip::address_v4::loopback(), 0));
    first.non_blocking(true);
    client       sender(context);
    auto         properties = transform();
    std::int32_t tick       = 0;
    auto         send       = [&](int channel, const monitor::state& value) {
        sender.send(channel, snapshot(channel, value, ++tick));
        return receive(first, tick);
    };

    sender.send(1, snapshot(1, properties, ++tick));
    auto first_token = sender.get_subscription_token(first.local_endpoint());
    require_transform(send(1, properties), 1, properties);
    auto idle = send(1, properties);
    require_transform(idle, 1, {});
    require(idle.contains(layer_path(1) + "/foreground/transform/opacity"),
            "Unrelated producer telemetry was suppressed");

    // Exercise every scalar/vector property without changing any renderer-only fields.
    for (const auto& p : transform()) {
        monitor::state change;
        if (p.second.size() == 2)
            change[p.first] = {0.25, 0.5};
        else
            change[p.first] = 0.25;
        properties[p.first] = change.begin()->second;
        require_transform(send(1, properties), 1, change);
        require_transform(send(1, properties), 1, {});
    }

    require_transform(send(2, properties), 2, properties);
    require_transform(send(1, properties), 1, {});

    monitor::state removed;
    removed["/test/tick"] = ++tick;
    sender.send(1, removed);
    receive(first, tick);
    require_transform(send(1, properties), 1, properties);
    require_transform(send(2, properties), 2, {});

    auto duplicate_token = sender.get_subscription_token(first.local_endpoint());
    require_transform(send(1, properties), 1, {});

    udp::socket second(*context, udp::endpoint(boost::asio::ip::address_v4::loopback(), 0));
    second.non_blocking(true);
    auto second_token = sender.get_subscription_token(second.local_endpoint());
    require_transform(send(1, properties), 1, properties);
    require_transform(receive(second, tick), 1, properties);
    require_transform(send(1, properties), 1, {});
    require_transform(receive(second, tick), 1, {});
    require_transform(send(2, properties), 2, properties);
    require_transform(receive(second, tick), 2, properties);
    second_token.reset();

    // Pending deltas must survive subsequent unchanged snapshots and other channels.
    for (int i = 0; i < 20; ++i) {
        properties["opacity"] = static_cast<double>(i) / 32;
        sender.send(1, snapshot(1, properties, ++tick));
    }
    sender.send(2, snapshot(2, properties, ++tick));
    sender.send(1, snapshot(1, properties, ++tick));
    const auto merged = receive(first, tick);
    require(merged.at(layer_path(1) + "/transform/opacity") == std::vector<float>{19.0f / 32},
            "Latest pending transform was lost");
    require_transform(send(1, properties), 1, {});

    first_token.reset();
    duplicate_token.reset();
    sender.send(1, snapshot(1, properties, ++tick));
    first_token = sender.get_subscription_token(first.local_endpoint());
    require_transform(send(1, properties), 1, properties);

    // Port zero fails locally. It sorts before the live endpoint, so receiving the
    // tick also confirms that the sender has invalidated its cache after the error.
    auto invalid_token = sender.get_subscription_token(udp::endpoint(boost::asio::ip::address_v4::loopback(), 0));
    require_transform(send(1, properties), 1, properties);
    require_transform(send(1, properties), 1, properties);
    invalid_token.reset();
    require_transform(send(1, properties), 1, properties);
    require_transform(send(1, properties), 1, {});

    // Force multiple bundles and verify that no transform message is dropped.
    monitor::state large;
    for (int layer = 0; layer < 40; ++layer)
        large[""]["channel"][3]["stage"]["layer"][layer]["transform"] = properties;
    large["/test/tick"] = ++tick;
    sender.send(3, large);
    const auto split = receive(first, tick);
    require(split.size() == 40 * 11, "Split OSC bundles lost transform messages");
}

} // namespace

int main()
{
    try {
        run();
        std::cout << "OSC transform queue and UDP loopback tests passed\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "OSC test failed: " << e.what() << '\n';
        return 1;
    }
}
