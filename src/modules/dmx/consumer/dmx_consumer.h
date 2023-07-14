/*
* Copyright (c) 2023 Eliyah Sundström
*
* This file is part of an extension of the CasparCG project
*
* Author: Eliyah Sundström eliyah@sundstroem.com
 */


#pragma once

#include <common/memory.h>

#include <core/consumer/frame_consumer.h>

#include <string>
#include <vector>

namespace caspar {
    namespace dmx {

        spl::shared_ptr<core::frame_consumer>
        create_consumer(const std::vector<std::wstring>&                         params,
                       const core::video_format_repository&                     format_repository,
                       const std::vector<spl::shared_ptr<core::video_channel>>& channels);

    }
} // namespace caspar::dmx
