/*
* Copyright (c) 2023 Eliyah Sundström
*
* This file is part of an extension of the CasparCG project
*
* Author: Eliyah Sundström eliyah@sundstroem.com
 */


#include "dmx_consumer.h"

#undef NOMINMAX
// ^^ This is needed to avoid a conflict between boost asio and other header files defining NOMINMAX

#include <common/array.h>
#include <common/future.h>
#include <common/log.h>
#include <common/param.h>
#include <common/utf.h>
#include <common/ptree.h>

#include <core/consumer/frame_consumer.h>
#include <core/frame/frame.h>
#include <core/video_format.h>

#include <boost/algorithm/string.hpp>
#include <boost/property_tree/ptree.hpp>

#include <boost/locale/encoding_utf.hpp>
#include <cmath>
#include <utility>
#include <vector>

#define M_PI           3.14159265358979323846  /* pi */

namespace caspar {
    namespace dmx {

        enum FixtureType {
            DIMMER = 1,
            RGB = 3,
            RGBW = 4,
        };

        struct point {
                float x;
                float y;
        };

        struct rect {
                point p1;
                point p2;
                point p3;
                point p4;
        };

        struct computed_fixture {
            FixtureType type;
            unsigned short address;

            rect rect;
        };

        struct color {
            std::uint8_t r;
            std::uint8_t g;
            std::uint8_t b;
        };


        struct box {
                float x;
                float y;

                float width;
                float height;

                float rotation; // degrees
        };

        struct fixture {
            FixtureType type;
            unsigned short startAddress; // DMX address of the first channel in the fixture
            unsigned short fixtureCount; // number of fixtures in the chain, dividing along the width

            box fixtureBox;
        };

        struct configuration
        {
            int universe = 0;
            std::wstring host = L"127.0.0.1";
            unsigned short port = 6454;

            int refreshRate = 10;

            std::vector<fixture> fixtures;
        };

        struct dmx_consumer : public core::frame_consumer
        {

            const configuration config;
            std::vector<computed_fixture> computed_fixtures;

         public:
                // frame_consumer

                explicit dmx_consumer(configuration config)
                    : config(std::move(config))
                {
                    compute_fixtures();
                    memset(dmx_data, 0, 512);
                }


                void initialize(const core::video_format_desc& /*format_desc*/, int /*channel_index*/) override {
                    thread_ = std::thread([this] {
                        auto port = config.port;
                        auto host = config.host;

                        auto universe = config.universe;

                        try {
                            auto last_send = std::chrono::system_clock::now();
                            while (!abort_request_) {
                                auto now = std::chrono::system_clock::now();
                                std::chrono::duration<double> elapsed_seconds = now - last_send;

                                long long time = 1000 / config.refreshRate;
                                if (std::chrono::duration_cast<std::chrono::milliseconds>(elapsed_seconds).count() > time) {
                                    send_dmx_data(port, host, universe, dmx_data, 512);
                                    last_send = now;
                                }
                            }
                        } catch (...) {
                            CASPAR_LOG_CURRENT_EXCEPTION();
                        }
                    });
                }

                ~dmx_consumer()
                {
                    abort_request_ = true;
                    if (thread_.joinable()) {
                            thread_.join();
                    }
                }

                std::future<bool> send(core::video_field field, core::const_frame frame) override
                {
                    std::thread async([frame, this] {
                        try {
                            for (auto computed_fixture : computed_fixtures) {
                                auto color = average_color(frame, computed_fixture.rect);
                                uint8_t* ptr = dmx_data + computed_fixture.address;

                                switch (computed_fixture.type) {
                                    case FixtureType::DIMMER:
                                        ptr[0] = (color.r + color.g + color.b) / 3;
                                        break;
                                    case FixtureType::RGB:
                                        ptr[0] = color.r;
                                        ptr[1] = color.g;
                                        ptr[2] = color.b;
                                        break;
                                    case FixtureType::RGBW:
                                        uint8_t w = std::min(std::min(color.r, color.g), color.b);
                                        ptr[0] = color.r - w;
                                        ptr[1] = color.g - w;
                                        ptr[2] = color.b - w;
                                        ptr[3] = w;
                                        break;
                                }
                            }
                        } catch (...) {
                            CASPAR_LOG_CURRENT_EXCEPTION();
                        }
                    });
                    async.detach();

                    return make_ready_future(true);
                }

                std::wstring print() const override { return L"dmx[]"; }

                std::wstring name() const override { return L"dmx"; }

                int index() const override { return 1337; }

                core::monitor::state state() const override
                {
                    core::monitor::state state;
                    return state;
                }

         private:

                std::thread       thread_;
                std::atomic<bool> abort_request_{false};

                uint8_t dmx_data[512];

                void compute_fixtures() {
                    computed_fixtures.clear();
                    for (auto fixture : config.fixtures) {
                        for (int i = 0; i < fixture.fixtureCount; i++) {
                            computed_fixture computed_fixture{};
                            computed_fixture.type = fixture.type;
                            computed_fixture.address = fixture.startAddress + i * fixture.type;

                            computed_fixture.rect = compute_rect(fixture.fixtureBox, i, fixture.fixtureCount);
                            computed_fixtures.push_back(computed_fixture);
                        }
                    }
                }

                static rect compute_rect(box fixtureBox, int index, int count)
                {
                    auto f_count = (float) count;
                    auto f_index = (float) index;

                    float x = fixtureBox.x;
                    float y = fixtureBox.y;

                    float width  = fixtureBox.width;
                    float height = fixtureBox.height;

                    float rotation = fixtureBox.rotation;

                    float angle = M_PI * rotation / 180.0f;

                    float dx = width / (2 * f_count);
                    float dy = height / 2.0f;

                    float sin_ = sin(angle);
                    float cos_ = cos(angle);

                    float od = (2 * f_index - f_count + 1) * dx;

                    float ox = x + od * cos_;
                    float oy = y + od * sin_;

                    point p1 {
                            ox + -dx *  cos_ + -dy * -sin_,
                            oy + -dx *  sin_ + -dy *  cos_,
                    };

                    point p2 {
                            ox +  dx *  cos_ + -dy * -sin_,
                            oy +  dx *  sin_ + -dy *  cos_,
                    };

                    point p3 {
                            ox +  dx *  cos_ +  dy * -sin_,
                            oy +  dx *  sin_ +  dy *  cos_,
                    };

                    point p4 {
                            ox + -dx *  cos_ +  dy * -sin_,
                            oy + -dx *  sin_ +  dy *  cos_,
                    };

                    rect rectangle {
                            p1,
                            p2,
                            p3,
                            p4
                    };

                    return rectangle;
                }

                static color average_color(const core::const_frame& frame, rect& rect) {
                    int width = (int) frame.width();
                    int height = (int) frame.height();

                    float y1 = rect.p1.y;
                    float y2 = rect.p2.y;
                    float y3 = rect.p3.y;
                    float y4 = rect.p4.y;

                    float x_values[4];
                    float y_values[] = {
                            y1,
                            y2,
                            y3,
                            y4
                    };

                    std::sort(std::begin(y_values), std::end(y_values));

                    for (int i = 0; i < 4;) {
                        int c = i;

                        if (y1 == y_values[i]) {
                            x_values[i] = rect.p1.x;
                            i++;
                        }
                        if (i >= 4) break;

                        if (y2 == y_values[i]) {
                            x_values[i] = rect.p2.x;
                            i++;
                        }
                        if (i >= 4) break;

                        if (y3 == y_values[i]) {
                            x_values[i] = rect.p3.x;
                            i++;
                        }
                        if (i >= 4) break;

                        if (y4 == y_values[i]) {
                            x_values[i] = rect.p4.x;
                            i++;
                        }
                        if (c == i) return color{0, 0, 0}; // should never happen, but prevents infinite loop in case of the impossible
                    }

                    const int indices[3][4] = {
                        {0, 1, 0, 2},
                        {0, 2, 1, 3},
                        {1, 3, 2, 3},
                    };

                    int y_min = std::max(0, std::min(height - 1, (int) y_values[0]));
                    int y_max = std::max(0, std::min(height - 1, (int) y_values[3]));

                    const array<const std::uint8_t>& values = frame.image_data(0);
                    const std::uint8_t* value_ptr = values.data();

                    unsigned long long tr = 0;
                    unsigned long long tg = 0;
                    unsigned long long tb = 0;

                    unsigned long long count = 0;

                    for (int y = y_min; y <= y_max; y++) {
                        int index = 0;
                        if (y >= (int) y_values[1]) index = 1;
                        if (y >= (int) y_values[2]) index = 2;

                        float ax1 = x_values[indices[index][0]];
                        float ay1 = y_values[indices[index][0]];

                        float bx1 = x_values[indices[index][1]];
                        float by1 = y_values[indices[index][1]];

                        float ax2 = x_values[indices[index][2]];
                        float ay2 = y_values[indices[index][2]];

                        float bx2 = x_values[indices[index][3]];
                        float by2 = y_values[indices[index][3]];

                        float d1 = (bx1 - ax1) / (by1 - ay1);
                        float d2 = (bx2 - ax2) / (by2 - ay2);

                        auto x1 = std::max(0, std::min(width - 1, (int) (ax1 + ((float) y - ay1) * d1)));
                        auto x2 = std::max(0, std::min(width - 1, (int) (ax2 + ((float) y - ay2) * d2)));

                        int min_x = std::min(x1, x2);
                        int max_x = std::max(x1, x2);

                        for (int x = min_x; x <= max_x; x++) {
                            int pos = y * width + x;
                            const std::uint8_t* base_ptr = value_ptr + pos * 4;

                            float a = (float) base_ptr[3] / 255.0f;

                            float r = (float) base_ptr[2] * a;
                            float g = (float) base_ptr[1] * a;
                            float b = (float) base_ptr[0] * a;

                            tr += (unsigned long long) (r);
                            tg += (unsigned long long) (g);
                            tb += (unsigned long long) (b);

                            count++;
                        }
                    }

                    color c {
                        (std::uint8_t) (tr / count),
                        (std::uint8_t) (tg / count),
                        (std::uint8_t) (tb / count)
                    };

                    return c;
                }
        };

        std::vector<fixture> get_fixtures_ptree(const boost::property_tree::wptree& ptree)
        {
                std::vector<fixture> fixtures;

                using boost::property_tree::wptree;

                for (auto& xml_channel : ptree | witerate_children(L"fixtures") | welement_context_iteration) {
                        ptree_verify_element_name(xml_channel, L"fixture");

                        fixture f {

                        };

                        int startAddress = xml_channel.second.get(L"start-address", 0);
                        if (startAddress < 1) throw std::runtime_error("Fixture start address must be specified");

                        f.startAddress = startAddress - 1;

                        int fixtureCount = xml_channel.second.get(L"fixture-count", -1);
                        if (fixtureCount < 0) throw std::runtime_error("Fixture count must be specified");

                        f.fixtureCount = fixtureCount;

                        std::wstring type = xml_channel.second.get(L"type", L"");
                        if (type.empty()) throw std::runtime_error("Fixture type must be specified, available types: DIMMER, RGB, RGBW");
                        if (boost::iequals(type, L"DIMMER")) {
                            f.type = FixtureType::DIMMER;
                        } else if (boost::iequals(type, L"RGB")) {
                            f.type = FixtureType::RGB;
                        } else if (boost::iequals(type, L"RGBW")) {
                            f.type = FixtureType::RGBW;
                        } else {
                            throw std::runtime_error("Unknown fixture type: " + boost::locale::conv::utf_to_utf<char>(type));
                        }

                        box b {};

                        auto x = xml_channel.second.get(L"x", 0.0f);
                        auto y = xml_channel.second.get(L"y", 0.0f);

                        b.x = x;
                        b.y = y;

                        auto width = xml_channel.second.get(L"width", 0.0f);
                        auto height = xml_channel.second.get(L"height", 0.0f);

                        b.width = width;
                        b.height = height;

                        auto rotation = xml_channel.second.get(L"rotation", 0.0f);

                        b.rotation = rotation;

                        f.fixtureBox = b;

                        fixtures.push_back(f);
                }

                return fixtures;
        }

        spl::shared_ptr<core::frame_consumer> create_consumer(const std::vector<std::wstring>&     params,
                                                             const core::video_format_repository& format_repository,
                                                             const std::vector<spl::shared_ptr<core::video_channel>>& channels)
        {
            return core::frame_consumer::empty(); // TODO implement
        }

        spl::shared_ptr<core::frame_consumer> create_preconfigured_consumer(const boost::property_tree::wptree&               ptree,
                                          const core::video_format_repository&              format_repository,
                                          std::vector<spl::shared_ptr<core::video_channel>> channels)
        {
           configuration config;

           config.universe     = ptree.get(L"universe", config.universe);
           config.host         = ptree.get(L"host", config.host);
           config.port         = ptree.get(L"port", config.port);
           config.refreshRate  = ptree.get(L"refresh-rate", config.refreshRate);

           config.fixtures     = get_fixtures_ptree(ptree);

           return spl::make_shared<dmx_consumer>(config);
        }
    }
} // namespace caspar::dmx
