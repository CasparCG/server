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
 * Author: Nicklas P Andersson
 */

#pragma once

#include "../util/protocol_strategy.h"
#include "clk_command_processor.h"
#include <core/producer/cg_proxy.h>
#include <core/video_channel.h>

namespace caspar { namespace protocol { namespace CLK {

class clk_protocol_strategy_factory : public IO::protocol_strategy_factory<wchar_t>
{
    clk_command_processor command_processor_;

  public:
    clk_protocol_strategy_factory(const std::vector<spl::shared_ptr<core::video_channel>>&    channels,
                                  const spl::shared_ptr<core::cg_producer_registry>&          cg_registry,
                                  const spl::shared_ptr<const core::frame_producer_registry>& producer_registry);

    IO::protocol_strategy<wchar_t>::ptr create(const IO::client_connection<wchar_t>::ptr& client_connection) override;
};

}}} // namespace caspar::protocol::CLK
