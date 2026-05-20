/*
 * Copyright (c) 2011 Sveriges Television AB <info@casparcg.com>
 *
 * This file is part of CasparCG (www.casparcg.com).
 *
 * CasparCG is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * CasparCG is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with CasparCG. If not, see <http://www.gnu.org/licenses/>.
 *
 * Author: Eliyah Sundström eliyah@sundstroem.com
 */

#include "artnet_consumer.h"

#undef NOMINMAX
// ^^ This is needed to avoid a conflict between boost asio and other header files defining NOMINMAX

#include <common/future.h>
#include <common/log.h>
#include <common/ptree.h>

#include <core/consumer/channel_info.h>
#include <core/frame/frame.h>
#include <core/video_format.h>

#include <boost/algorithm/string.hpp>
#include <boost/asio.hpp>
#include <boost/property_tree/ptree.hpp>

extern "C" {
#define __STDC_CONSTANT_MACROS
#define __STDC_LIMIT_MACROS
#include <libavutil/pixfmt.h>
#include <libswscale/swscale.h>
}

#include <algorithm>
#include <cstring>
#include <memory>
#include <thread>
#include <utility>
#include <vector>

using namespace boost::asio;
using namespace boost::asio::ip;

namespace caspar { namespace artnet {

struct configuration
{
    int            universe = 0;
    std::wstring   host     = L"127.0.0.1";
    unsigned short port     = 6454;

    int refreshRate = 10;

    std::vector<fixture> fixtures;
};

struct sub_fixture
{
    FixtureType    type;
    unsigned short address;
};

// One group corresponds to one <fixture> in config: a chain of N sub-fixtures sharing a box.
// sws_scale produces a 1 x N BGRA strip in `output`, one pixel per sub-fixture.
struct computed_fixture_group
{
    box                         source_box{};
    int                         crop_x = 0;
    int                         crop_y = 0;
    int                         crop_w = 0;
    int                         crop_h = 0;
    std::shared_ptr<SwsContext> sws;
    std::vector<std::uint8_t>   output;
    std::vector<sub_fixture>    sub_fixtures;
};

struct artnet_consumer : public core::frame_consumer
{
    const configuration                 config;
    std::vector<computed_fixture_group> fixture_groups;

  public:
    explicit artnet_consumer(configuration config)
        : config(std::move(config))
        , io_context_()
        , socket(io_context_)
    {
        socket.open(udp::v4());

        std::string host_ = u8(this->config.host);
        remote_endpoint   = boost::asio::ip::udp::endpoint(boost::asio::ip::make_address(host_), this->config.port);

        build_fixture_groups();
    }

    void initialize(const core::video_format_desc& format_desc,
                    const core::channel_info& /*channel_info*/,
                    int /*port_index*/) override
    {
        src_width_    = format_desc.width;
        src_height_   = format_desc.height;
        src_linesize_ = src_width_ * 4;

        for (auto& group : fixture_groups)
            setup_group_sws(group);

        thread_ = std::thread([this] {
            long long time      = 1000 / config.refreshRate;
            auto      last_send = std::chrono::system_clock::now();

            while (!abort_request_) {
                try {
                    auto                          now             = std::chrono::system_clock::now();
                    std::chrono::duration<double> elapsed_seconds = now - last_send;
                    long long                     elapsed_ms =
                        std::chrono::duration_cast<std::chrono::milliseconds>(elapsed_seconds).count();

                    long long sleep_time = time - elapsed_ms;
                    if (sleep_time > 0)
                        std::this_thread::sleep_for(std::chrono::milliseconds(sleep_time));

                    last_send = now;

                    frame_mutex_.lock();
                    auto frame = last_frame_;
                    frame_mutex_.unlock();
                    if (!frame)
                        continue;

                    if ((int)frame.width() != src_width_ || (int)frame.height() != src_height_)
                        continue;

                    std::uint8_t dmx_data[512];
                    std::memset(dmx_data, 0, sizeof(dmx_data));

                    const std::uint8_t* src = frame.image_data(0).data();

                    for (auto& group : fixture_groups) {
                        if (!group.sws)
                            continue;

                        const std::uint8_t* crop_src    = src + group.crop_y * src_linesize_ + group.crop_x * 4;
                        const std::uint8_t* src_data[1] = {crop_src};
                        int                 src_lines[1] = {src_linesize_};

                        std::uint8_t* dst_data[1]  = {group.output.data()};
                        int           dst_lines[1] = {(int)(group.sub_fixtures.size() * 4)};

                        sws_scale(group.sws.get(), src_data, src_lines, 0, group.crop_h, dst_data, dst_lines);

                        for (size_t i = 0; i < group.sub_fixtures.size(); i++) {
                            const auto&         sf = group.sub_fixtures[i];
                            const std::uint8_t* px = group.output.data() + i * 4;
                            std::uint8_t        b  = px[0];
                            std::uint8_t        g  = px[1];
                            std::uint8_t        r  = px[2];

                            std::uint8_t* ptr = dmx_data + sf.address;
                            switch (sf.type) {
                                case FixtureType::DIMMER:
                                    ptr[0] = (std::uint8_t)(0.279 * r + 0.547 * g + 0.106 * b);
                                    break;
                                case FixtureType::RGB:
                                    ptr[0] = r;
                                    ptr[1] = g;
                                    ptr[2] = b;
                                    break;
                                case FixtureType::RGBW: {
                                    std::uint8_t w = std::min({r, g, b});
                                    ptr[0]         = r - w;
                                    ptr[1]         = g - w;
                                    ptr[2]         = b - w;
                                    ptr[3]         = w;
                                    break;
                                }
                            }
                        }
                    }

                    send_dmx_data(dmx_data, 512);
                } catch (...) {
                    CASPAR_LOG_CURRENT_EXCEPTION();
                }
            }
        });
    }

    ~artnet_consumer()
    {
        abort_request_ = true;
        if (thread_.joinable())
            thread_.join();
    }

    std::future<bool> send(core::video_field field, core::const_frame frame) override
    {
        std::lock_guard<std::mutex> lock(frame_mutex_);
        last_frame_ = frame;

        return make_ready_future(true);
    }

    std::wstring print() const override { return L"artnet[]"; }

    std::wstring name() const override { return L"artnet"; }

    int index() const override { return 1337; }

    core::monitor::state state() const override
    {
        core::monitor::state state;
        std::size_t          total = 0;
        for (const auto& g : fixture_groups)
            total += g.sub_fixtures.size();
        state["artnet/computed-fixtures"] = total;
        state["artnet/fixtures"]          = config.fixtures.size();
        state["artnet/universe"]          = config.universe;
        state["artnet/host"]              = config.host;
        state["artnet/port"]              = config.port;
        state["artnet/refresh-rate"]      = config.refreshRate;

        return state;
    }

  private:
    core::const_frame last_frame_;
    std::mutex        frame_mutex_;

    std::thread       thread_;
    std::atomic<bool> abort_request_{false};

    int src_width_    = 0;
    int src_height_   = 0;
    int src_linesize_ = 0;

    io_context    io_context_;
    udp::socket   socket;
    udp::endpoint remote_endpoint;

    void build_fixture_groups()
    {
        fixture_groups.clear();
        for (const auto& fx : config.fixtures) {
            computed_fixture_group group;
            group.source_box = fx.fixtureBox;
            group.sub_fixtures.reserve(fx.fixtureCount);
            for (unsigned short i = 0; i < fx.fixtureCount; i++) {
                sub_fixture sf{};
                sf.type    = fx.type;
                sf.address = fx.startAddress + i * fx.fixtureChannels;
                group.sub_fixtures.push_back(sf);
            }
            fixture_groups.push_back(std::move(group));
        }
    }

    void setup_group_sws(computed_fixture_group& group)
    {
        int x0 = std::max(0, (int)group.source_box.x);
        int y0 = std::max(0, (int)group.source_box.y);
        int x1 = std::min(src_width_, (int)(group.source_box.x + group.source_box.width));
        int y1 = std::min(src_height_, (int)(group.source_box.y + group.source_box.height));

        group.crop_x = x0;
        group.crop_y = y0;
        group.crop_w = x1 - x0;
        group.crop_h = y1 - y0;

        if (group.crop_w <= 0 || group.crop_h <= 0 || group.sub_fixtures.empty()) {
            CASPAR_LOG(warning) << L"artnet: fixture box is empty or outside the channel; skipping.";
            return;
        }

        int dst_w = (int)group.sub_fixtures.size();
        group.sws.reset(sws_getContext(group.crop_w,
                                       group.crop_h,
                                       AV_PIX_FMT_BGRA,
                                       dst_w,
                                       1,
                                       AV_PIX_FMT_BGRA,
                                       SWS_AREA,
                                       nullptr,
                                       nullptr,
                                       nullptr),
                        [](SwsContext* p) {
                            if (p)
                                sws_freeContext(p);
                        });

        if (!group.sws)
            CASPAR_THROW_EXCEPTION(caspar_exception() << msg_info("artnet: failed to create SwsContext"));

        group.output.assign(dst_w * 4, 0);
    }

    void send_dmx_data(const std::uint8_t* data, std::size_t length)
    {
        int universe = this->config.universe;

        std::uint8_t hUni = (universe >> 8) & 0xff;
        std::uint8_t lUni = universe & 0xff;

        std::uint8_t hLen = (length >> 8) & 0xff;
        std::uint8_t lLen = (length & 0xff);

        std::uint8_t header[] = {65, 114, 116, 45, 78, 101, 116, 0, 0, 80, 0, 14, 0, 0, lUni, hUni, hLen, lLen};
        std::uint8_t buffer[18 + 512];

        for (int i = 0; i < 18 + 512; i++) {
            if (i < 18) {
                buffer[i] = header[i];
                continue;
            }

            if (i - 18 < length) {
                buffer[i] = data[i - 18];
                continue;
            }

            buffer[i] = 0;
        }

        boost::system::error_code err;
        socket.send_to(boost::asio::buffer(buffer), remote_endpoint, 0, err);
        CASPAR_LOG(trace) << "Sent DMX data to Artnet, universe: " << universe;
        if (err)
            CASPAR_THROW_EXCEPTION(io_error() << msg_info(err.message()));
    }
};

std::vector<fixture> get_fixtures_ptree(const boost::property_tree::wptree& ptree)
{
    std::vector<fixture> fixtures;

    using boost::property_tree::wptree;

    for (auto& xml_channel : ptree | witerate_children(L"fixtures") | welement_context_iteration) {
        ptree_verify_element_name(xml_channel, L"fixture");

        fixture f{};

        int startAddress = xml_channel.second.get(L"start-address", 0);
        if (startAddress < 1)
            CASPAR_THROW_EXCEPTION(user_error() << msg_info(L"Fixture start address must be specified"));

        f.startAddress = (unsigned short)startAddress - 1;

        int fixtureCount = xml_channel.second.get(L"fixture-count", -1);
        if (fixtureCount < 1)
            CASPAR_THROW_EXCEPTION(user_error() << msg_info(L"Fixture count must be specified"));

        f.fixtureCount = (unsigned short)fixtureCount;

        std::wstring type = xml_channel.second.get(L"type", L"");
        if (type.empty())
            CASPAR_THROW_EXCEPTION(user_error() << msg_info(L"Fixture type must be specified"));

        if (boost::iequals(type, L"DIMMER")) {
            f.type = FixtureType::DIMMER;
        } else if (boost::iequals(type, L"RGB")) {
            f.type = FixtureType::RGB;
        } else if (boost::iequals(type, L"RGBW")) {
            f.type = FixtureType::RGBW;
        } else {
            CASPAR_THROW_EXCEPTION(user_error() << msg_info(L"Unknown fixture type"));
        }

        int fixtureChannels = xml_channel.second.get(L"fixture-channels", -1);
        if (fixtureChannels < 0)
            fixtureChannels = f.type;
        if (fixtureChannels < f.type)
            CASPAR_THROW_EXCEPTION(
                user_error() << msg_info(
                    L"Fixture channel count must be at least enough channels for current color mode"));

        f.fixtureChannels = (unsigned short)fixtureChannels;

        if (xml_channel.second.get_optional<float>(L"rotation"))
            CASPAR_LOG(warning)
                << L"artnet: fixture <rotation> is no longer supported and will be ignored. "
                   L"Use the mixer ROTATION command on the source channel instead.";

        // Position can be given as center (<x>/<y>), top-left edge (<left>/<top>), or
        // bottom-right edge (<right>/<bottom>). Size is taken from <width>/<height>, or
        // derived from <left>+<right> / <top>+<bottom> when the explicit size is omitted.
        // Center takes priority, then left/top, then right/bottom.
        auto width_opt  = xml_channel.second.get_optional<float>(L"width");
        auto height_opt = xml_channel.second.get_optional<float>(L"height");
        auto cx         = xml_channel.second.get_optional<float>(L"x");
        auto left       = xml_channel.second.get_optional<float>(L"left");
        auto right      = xml_channel.second.get_optional<float>(L"right");
        auto cy         = xml_channel.second.get_optional<float>(L"y");
        auto top        = xml_channel.second.get_optional<float>(L"top");
        auto bottom     = xml_channel.second.get_optional<float>(L"bottom");

        bool derive_width  = !width_opt && !cx && left && right;
        bool derive_height = !height_opt && !cy && top && bottom;

        box b{};
        b.width  = derive_width ? *right - *left : width_opt.value_or(0.0f);
        b.height = derive_height ? *bottom - *top : height_opt.value_or(0.0f);

        if (((cx ? 1 : 0) + (left ? 1 : 0) + (right ? 1 : 0)) > 1 && !derive_width)
            CASPAR_LOG(warning)
                << L"artnet: fixture has conflicting horizontal specifiers. "
                   L"Use one of <x>/<left>/<right> with <width>, or <left>+<right>. "
                   L"Using <x> if present, otherwise <left>, otherwise <right>.";

        if (((cy ? 1 : 0) + (top ? 1 : 0) + (bottom ? 1 : 0)) > 1 && !derive_height)
            CASPAR_LOG(warning)
                << L"artnet: fixture has conflicting vertical specifiers. "
                   L"Use one of <y>/<top>/<bottom> with <height>, or <top>+<bottom>. "
                   L"Using <y> if present, otherwise <top>, otherwise <bottom>.";

        if (cx)
            b.x = *cx - b.width / 2.0f;
        else if (left)
            b.x = *left;
        else if (right)
            b.x = *right - b.width;
        else
            b.x = 0.0f;

        if (cy)
            b.y = *cy - b.height / 2.0f;
        else if (top)
            b.y = *top;
        else if (bottom)
            b.y = *bottom - b.height;
        else
            b.y = 0.0f;

        f.fixtureBox = b;

        fixtures.push_back(f);
    }

    return fixtures;
}

spl::shared_ptr<core::frame_consumer>
create_preconfigured_consumer(const boost::property_tree::wptree&                      ptree,
                              const core::video_format_repository&                     format_repository,
                              const std::vector<spl::shared_ptr<core::video_channel>>& channels,
                              const core::channel_info&                                channel_info)
{
    configuration config;

    if (channel_info.depth != common::bit_depth::bit8)
        CASPAR_THROW_EXCEPTION(caspar_exception() << msg_info("Artnet consumer only supports 8-bit color depth."));

    config.universe    = ptree.get(L"universe", config.universe);
    config.host        = ptree.get(L"host", config.host);
    config.port        = ptree.get(L"port", config.port);
    config.refreshRate = ptree.get(L"refresh-rate", config.refreshRate);

    if (config.refreshRate < 1)
        CASPAR_THROW_EXCEPTION(user_error() << msg_info(L"Refresh rate must be at least 1"));

    config.fixtures = get_fixtures_ptree(ptree);

    return spl::make_shared<artnet_consumer>(config);
}
}} // namespace caspar::artnet
