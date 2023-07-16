/*
* Copyright (c) 2023 Eliyah Sundström
*
* This file is part of an extension of the CasparCG project
*
* Author: Eliyah Sundström eliyah@sundstroem.com
 */


#include "dmx_consumer.h"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#if defined(_MSC_VER)
#include <windows.h>
#endif

#include <common/array.h>
#include <common/env.h>
#include <common/except.h>
#include <common/future.h>
#include <common/log.h>
#include <common/utf.h>

#include <core/consumer/frame_consumer.h>
#include <core/frame/frame.h>
#include <core/video_format.h>

#include <boost/algorithm/string.hpp>

#include <algorithm>
#include <utility>
#include <vector>

namespace caspar {
    namespace dmx {

        struct dmx_consumer : public core::frame_consumer
        {

         public:
                // frame_consumer

                explicit dmx_consumer()
                {
                }

                void initialize(const core::video_format_desc& /*format_desc*/, int /*channel_index*/) override {}

                std::future<bool> send(core::video_field field, core::const_frame frame) override
                {
                    std::thread async([frame] {
                        try {
                            array<const std::uint8_t> values = frame.image_data(0);
                            const std::uint8_t* value_ptr = values.data();
                            std::vector<std::string> colors;
                            std::vector<std::uint8_t> colors_dmx;

                            for (int i = 0; i < 10; i++) {
                                const std::uint8_t* base_ptr = value_ptr + i * 4;

                                const std::uint8_t* r_ptr = base_ptr + 0;
                                const std::uint8_t* g_ptr = base_ptr + 1;
                                const std::uint8_t* b_ptr = base_ptr + 2;

                                std::uint8_t r = *r_ptr;
                                std::uint8_t g = *g_ptr;
                                std::uint8_t b = *b_ptr;

                                colors_dmx.push_back(r);
                                colors_dmx.push_back(g);
                                colors_dmx.push_back(b);

                                int value = (r << 16) | (g << 8) | b;

                                std::stringstream ss;
                                ss << "#";
                                ss << std::hex << value;

                                colors.push_back(ss.str());
                            }

                            std::string color_str = boost::join(colors, L" ");
                            CASPAR_LOG(info) << L" Sending " << color_str;

                            send_dmx_data(6454, "127.0.0.1", 0, colors_dmx);
                        } catch (...) {
                            CASPAR_LOG_CURRENT_EXCEPTION();
                        }
                    });
                    async.detach();

                    return make_ready_future(false);
                }

                std::wstring print() const override { return L"dmx[]"; }

                std::wstring name() const override { return L"dmx"; }

                int index() const override { return 1337; }

                core::monitor::state state() const override
                {
                    core::monitor::state state;
                    return state;
                }
        };

        spl::shared_ptr<core::frame_consumer> create_consumer(const std::vector<std::wstring>&     params,
                                                             const core::video_format_repository& format_repository,
                                                             const std::vector<spl::shared_ptr<core::video_channel>>& channels)
        {
           if (params.empty() || !boost::iequals(params.at(0), L"DMX"))
               return core::frame_consumer::empty();

           return spl::make_shared<dmx_consumer>();
        }

    }
} // namespace caspar::dmx
