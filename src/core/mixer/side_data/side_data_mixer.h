/*
 * Copyright (c) 2025 Jacob Lifshay <programmerjake@gmail.com>
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
 */

#pragma once

#include <core/frame/frame_side_data.h>
#include <core/frame/frame_visitor.h>

#include <vector>

namespace caspar::core {

class side_data_mixer final : public frame_visitor
{
  public:
    side_data_mixer(const side_data_mixer&)            = delete;
    side_data_mixer& operator=(const side_data_mixer&) = delete;
    explicit side_data_mixer(spl::shared_ptr<diagnostics::graph> graph);
    ~side_data_mixer();

    frame_side_data_in_queue mixed();

    void push(const frame_transform& transform) override;
    void visit(const const_frame& frame) override;
    void pop() override;

  private:
    struct impl;
    std::unique_ptr<impl> impl_;
};

} // namespace caspar::core
