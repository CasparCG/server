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
#include <core/frame/frame_side_data.h>
#include <core/video_format.h>
#include <memory>

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
    virtual vanc_packet         pop_packet(bool field2)                                = 0;
    virtual bool                try_push_data(const std::vector<std::wstring>& params) = 0;
    virtual const std::wstring& get_name() const                                       = 0;
};

class decklink_frame_side_data_vanc_strategy : public decklink_vanc_strategy
{
  public:
    const core::frame_side_data_type type;
    explicit decklink_frame_side_data_vanc_strategy(core::frame_side_data_type type)
        : type(type)
    {
    }
    virtual bool try_push_data(const std::vector<std::wstring>& params) override
    {
        static_cast<void>(params);
        return false;
    }
    virtual void push_frame_side_data(const core::frame_side_data_in_queue& field1_side_data,
                                      const core::frame_side_data_in_queue& field2_side_data) = 0;
    /// tries to create a `decklink_frame_side_data_vanc_strategy` instance for the specified side-data type,
    /// returns `nullptr` if there is no corresponding conversion
    static std::shared_ptr<decklink_frame_side_data_vanc_strategy> try_create(core::frame_side_data_type     type,
                                                                              const core::video_format_desc& format);
};

class decklink_vanc
{
    std::vector<std::shared_ptr<decklink_vanc_strategy>>                 strategies_;
    std::vector<std::shared_ptr<decklink_frame_side_data_vanc_strategy>> side_data_strategies_;

  public:
    explicit decklink_vanc(const vanc_configuration& config, const core::video_format_desc& format);
    bool                                                             has_data() const;
    std::vector<caspar::decklink::com_ptr<IDeckLinkAncillaryPacket>> pop_packets(bool field2 = false);
    bool try_push_data(const std::vector<std::wstring>& params);
    void push_frame_side_data(const core::frame_side_data_in_queue& field1_side_data,
                              const core::frame_side_data_in_queue& field2_side_data);
};

std::shared_ptr<decklink_vanc_strategy>
create_op47_strategy(uint8_t line_number, uint8_t line_number_2, const std::wstring& dummy_header);
std::shared_ptr<decklink_vanc_strategy> create_scte104_strategy(uint8_t line_number);

std::shared_ptr<decklink_vanc> create_vanc(const vanc_configuration& config, const core::video_format_desc& format);
}} // namespace caspar::decklink