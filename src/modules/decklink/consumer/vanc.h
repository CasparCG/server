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
 * Author: Niklas Andersson, niklas.andersson@nxtedition.com
 */

#pragma once
#include "../decklink_api.h"
#include "config.h"
#include <common/memory.h>

namespace caspar { namespace decklink {

struct vanc_packet
{
    uint8_t              did;
    uint8_t              sdid;
    uint32_t             line_number;
    std::vector<uint8_t> data;
};

class decklink_vanc_strategy
{
  public:
    virtual ~decklink_vanc_strategy() noexcept = default;

    virtual bool                has_data() const                                       = 0;
    virtual vanc_packet         pop_packet()                                           = 0;
    virtual bool                try_push_data(const std::vector<std::wstring>& params) = 0;
    virtual const std::wstring& get_name() const                                       = 0;
};

class decklink_vanc
{
    std::vector<std::shared_ptr<decklink_vanc_strategy>> strategies_;

  public:
    explicit decklink_vanc(const vanc_configuration& config);
    bool                                                             has_data() const;
    std::vector<caspar::decklink::com_ptr<IDeckLinkAncillaryPacket>> pop_packets();
    bool try_push_data(const std::vector<std::wstring>& params);
};

std::shared_ptr<decklink_vanc_strategy> create_scte104_strategy(uint8_t line_number);

std::shared_ptr<decklink_vanc> create_vanc(const vanc_configuration& config);
}} // namespace caspar::decklink