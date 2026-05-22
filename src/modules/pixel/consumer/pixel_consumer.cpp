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
 * Author: Dimitry Ishenko, dimitry (dot) ishenko (at) (gee) mail (dot) com
 */

#include "pixel_consumer.h"

#include "common/endian.h"
#include "common/executor.h"
#include "common/future.h"

#include "core/consumer/channel_info.h"
#include "core/consumer/frame_consumer.h"
#include "core/frame/frame.h"

#include <boost/asio.hpp>
#include <boost/property_tree/ptree.hpp>

#include <algorithm> // std::ranges::transform
#include <array>
#include <bit> // std::bit_cast
#include <cctype> // ::tolower
#include <functional>
#include <utility>
#include <vector>

#pragma warning(push)
#pragma warning(disable : 4267)

using namespace boost::asio;
using namespace boost::asio::ip;

namespace caspar::pixel {

////////////////////
using level = std::uint8_t;
struct pixel { level b, g, r, a; };

class artdmx_sink
{
#pragma pack(push,1)
    struct
    {
        const char id[8] = "Art-Net";
        const std::uint16_t opcode  = little_endian(0x5000); // ArtDMX
        const std::uint16_t protver = big_endian<std::uint16_t>(14); // v14
        const std::uint8_t  seq = 0; // ignore
        const std::uint8_t  phy = 0; // ignore

        std::uint16_t universe; // little endian
        std::uint16_t length;   // big endian
    }
    head_;
#pragma pack(pop)

    std::vector<level> payload_;

    std::uint16_t universe_;
    udp::socket& socket_;

    void send()
    {
        head_.universe = little_endian(universe_);
        head_.length = big_endian<std::uint16_t>(payload_.size());

        boost::system::error_code ec;
        socket_.send(std::array{
            boost::asio::buffer(&head_, sizeof(head_)),
            boost::asio::buffer(payload_)
        }, 0, ec);
        if (ec) CASPAR_THROW_EXCEPTION(io_error() << msg_info(ec.message()));
    }

public:
    // Invariants:
    // 1. unverse < 32768
    // 2. address < 512
    artdmx_sink(std::uint16_t universe, std::uint16_t address, udp::socket& socket) :
        universe_{universe}, socket_{socket}
    {
        payload_.reserve(512);
        payload_.resize(address, 0);
    }

    ~artdmx_sink() { if (payload_.size()) send(); }

    void push(level val)
    {
        if (universe_ < 32768) {
            payload_.push_back(val);
            if (payload_.size() == 512) {
                send();
                ++universe_;
                payload_.clear();
            }
        }
    }
};

////////////////////
class color_grader
{
    float cr_, cg_, cb_, cw_;
    std::array<level, 256> gm_;

    constexpr auto grade(pixel p) const {
        return std::tuple{ cr_ * gm_[p.r], cg_ * gm_[p.g], cb_ * gm_[p.b] };
    }

    static constexpr auto round(float v) { return std::clamp<level>(v + 0.5f, 0, 255); }

public:
    constexpr color_grader() = default;
    constexpr color_grader(float cr, float cg, float cb, float cw, float gamma)
    {
        // normalize on rgb, and scale w
        auto min = std::min({ cr, cg, cb });
        cr_ = min / cr;
        cg_ = min / cg;
        cb_ = min / cb;
        cw_ = min / cw;

        for (auto i = 0; i < 256; ++i) gm_[i] = round(255.0f * std::pow(i / 255.0f, gamma));
    }

    constexpr auto to_y(pixel p) const
    {
        auto [ r, g, b ] = grade(p);
        // standard luma weights
        return round(0.2126f * r + 0.7152f * g + 0.0722f * b);
    }

    constexpr auto to_rgb(pixel p) const
    {
        auto [ r, g, b ] = grade(p);
        return std::array{ round(r), round(g), round(b) };
    }

    constexpr auto to_rgbw(pixel p) const
    {
        auto [ r, g, b ] = grade(p);
        auto y = std::min({ r, g, b });
        return std::array{ round(r - y), round(g - y), round(b - y), round(cw_ * y) };
    }

    constexpr auto to_rgbx(pixel p) const
    {
        auto [ r, g, b ] = grade(p);
        return std::array{ round(r), round(g), round(b), level{} };
    }
};

////////////////////
struct configuration
{
    std::uint16_t universe;
    std::uint16_t address;

    std::string   host;
    std::uint16_t port;

    color_grader  grader;
    std::function<void(artdmx_sink&, const color_grader&, pixel)> pusher;
};

struct pixel_consumer : public core::frame_consumer
{
    io_context    io_context_;
    udp::socket   socket_;

    configuration config_;
    executor      executor_{L"pixel_consumer"};

    int           channel_index_ = -1;
    core::video_format_desc format_desc_;

  public:
    explicit pixel_consumer(configuration config) :
        socket_{io_context_},
        config_{std::move(config)}
    {
        udp::endpoint remote{ make_address(config_.host), config_.port };

        socket_.open(udp::v4());
        socket_.connect(remote);
    }

    void initialize(const core::video_format_desc& format_desc,
                    const core::channel_info&      channel_info,
                    int                            port_index) override
    {
        channel_index_ = channel_info.index;
        format_desc_   = format_desc;
    }

    std::future<bool> send(core::video_field field, core::const_frame frame) override
    {
        executor_.begin_invoke([this, frame = std::move(frame)]{
            auto pix = std::bit_cast<const pixel*>(frame.image_data(0).data());
            auto size = frame.image_data(0).size() / sizeof(pixel);

            artdmx_sink sink{config_.universe, config_.address, socket_};
            for (std::size_t i = 0; i < size; ++i) config_.pusher(sink, config_.grader, pix[i]);
        });

        return make_ready_future(true);
    }

    std::wstring print() const override
    {
        return name() + L'[' + std::to_wstring(channel_index_) + L'|' + format_desc_.name + L']';
    }

    std::wstring name() const override { return L"pixel"; }

    int index() const override { return 1000; }

    core::monitor::state state() const override
    {
        static const core::monitor::state empty;
        return empty;
    }
};

spl::shared_ptr<core::frame_consumer>
create_preconfigured_consumer(const boost::property_tree::wptree&                      ptree,
                              const core::video_format_repository&                     format_repository,
                              const std::vector<spl::shared_ptr<core::video_channel>>& channels,
                              const core::channel_info&                                channel_info)
{
    if (channel_info.depth != common::bit_depth::bit8)
        CASPAR_THROW_EXCEPTION(caspar_exception() << msg_info("Unsupported color depth."));

    auto tolower = [](std::string s) {
        std::ranges::transform(s, s.begin(), ::tolower);
        return s;
    };

    configuration config;

    auto protocol = tolower(u8(ptree.get(L"protocol", L"")));
    if (protocol != "artnet")
        CASPAR_THROW_EXCEPTION(user_error() << msg_info("Unsupported or unspecified protocol."));

    config.host = u8(ptree.get(L"host", L"127.0.0.1"));
    config.port = ptree.get(L"port", 6454);

    config.universe = ptree.get(L"universe", 0);
    if (config.universe >= 32768) CASPAR_THROW_EXCEPTION(user_error() << msg_info("Invalid universe."));

    // start address is 1-based for the user (DMX spec), but 0-based for us
    config.address = ptree.get(L"start-address", 1) - 1;
    if (config.address >= 512) CASPAR_THROW_EXCEPTION(user_error() << msg_info("Invalid start address."));

    auto type = tolower(u8(ptree.get(L"type", L"")));
    if (type == "mono")
        config.pusher = [](auto& sink, auto& grader, auto pix){ sink.push(grader.to_y(pix)); };
    else if (type == "rgb")
        config.pusher = [](auto& sink, auto& grader, auto pix){ for (auto c : grader.to_rgb(pix)) sink.push(c); };
    else if (type == "rgbw")
        config.pusher = [](auto& sink, auto& grader, auto pix){ for (auto c : grader.to_rgbw(pix)) sink.push(c); };
    else if (type == "rgbx")
        config.pusher = [](auto& sink, auto& grader, auto pix){ for (auto c : grader.to_rgbx(pix)) sink.push(c); };
    else CASPAR_THROW_EXCEPTION(user_error() << msg_info("Unsupported or unspecified pixel type."));

    auto cr = ptree.get(L"coef.r", 1.0f);
    auto cg = ptree.get(L"coef.g", 1.0f);
    auto cb = ptree.get(L"coef.b", 1.0f);
    auto cw = ptree.get(L"coef.w", 1.0f);
    if (0 >= std::min({ cr, cg, cb, cw }))
        CASPAR_THROW_EXCEPTION(user_error() << msg_info("Invalid color correction coefficient(s)."));

    auto gamma = ptree.get(L"gamma", 1.0f);
    if (gamma < 0.1f || gamma > 10.0f) CASPAR_THROW_EXCEPTION(user_error() << msg_info("Invalid gamma."));

    config.grader = color_grader{cr, cg, cb, cw, 1};

    return spl::make_shared<pixel_consumer>(config);
}

} // namespace caspar::pixel
