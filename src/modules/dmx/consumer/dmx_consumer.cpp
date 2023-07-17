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

#include <core/consumer/frame_consumer.h>
#include <core/frame/frame.h>
#include <core/video_format.h>

#include <boost/algorithm/string.hpp>
#include <boost/property_tree/ptree.hpp>

#include <utility>
#include <vector>

namespace caspar {
    namespace dmx {

        struct configuration
        {
            int universe = 0;
            std::wstring host = L"127.0.0.1";
            unsigned short port = 6454;
        };

        struct dmx_consumer : public core::frame_consumer
        {

            const std::wstring      host_;
            const unsigned short      port_;
            const int      universe_;

         public:
                // frame_consumer

                dmx_consumer(configuration config)
                    : universe_(config.universe)
                    , host_(std::move(config.host))
                    , port_(config.port)
                {

                }


                void initialize(const core::video_format_desc& /*format_desc*/, int /*channel_index*/) override {}

                std::future<bool> send(core::video_field field, core::const_frame frame) override
                {
                    auto port = port_;
                    auto host = host_;

                    auto universe = universe_;

                    std::thread async([frame, port, host, universe] {
                        try {
                            array<const std::uint8_t> values = frame.image_data(0);
                            const std::uint8_t* value_ptr = values.data();
                            std::vector<std::uint8_t> colors;

                            for (int i = 0; i < 10; i++) {
                                const std::uint8_t* base_ptr = value_ptr + i * 4;

                                const std::uint8_t* r_ptr = base_ptr + 0;
                                const std::uint8_t* g_ptr = base_ptr + 1;
                                const std::uint8_t* b_ptr = base_ptr + 2;

                                std::uint8_t r = *r_ptr;
                                std::uint8_t g = *g_ptr;
                                std::uint8_t b = *b_ptr;

                                colors.push_back(r);
                                colors.push_back(g);
                                colors.push_back(b);
                            }

                            send_dmx_data(port, host, universe, colors);
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
        };

        spl::shared_ptr<core::frame_consumer> create_consumer(const std::vector<std::wstring>&     params,
                                                             const core::video_format_repository& format_repository,
                                                             const std::vector<spl::shared_ptr<core::video_channel>>& channels)
        {
           if (params.empty() || !boost::iequals(params.at(0), L"DMX"))
               return core::frame_consumer::empty();

           configuration config;

           config.universe     = get_param(L"UNIVERSE", params, 0);
           config.host         = get_param(L"HOST", params, L"");
           config.port         = get_param(L"PORT", params, 0);

           return spl::make_shared<dmx_consumer>(config);
        }

        spl::shared_ptr<core::frame_consumer> create_preconfigured_consumer(const boost::property_tree::wptree&               ptree,
                                          const core::video_format_repository&              format_repository,
                                          std::vector<spl::shared_ptr<core::video_channel>> channels)
        {
           configuration config;

           config.universe     = ptree.get(L"universe", config.universe);
           config.host         = ptree.get(L"host", config.host);
           config.port         = ptree.get(L"port", config.port);

           return spl::make_shared<dmx_consumer>(config);
        }
    }
} // namespace caspar::dmx
