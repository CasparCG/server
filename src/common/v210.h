/*
 * Copyright (c) 2026 Sveriges Television AB <info@casparcg.com>
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
 * Original Author: Niklas Andersson, niklas@nxtedition.com
 * Moved to common on 26/6/26
 * @todo - option for v210 with straight alpha
 */

#include <core/frame/frame.h>
#include <core/frame/pixel_format.h>
#include <core/video_format.h>

namespace caspar { namespace v210 {

struct output_region
{
    int src_x    = 0;
    int src_y    = 0;
    int dest_x   = 0;
    int dest_y   = 0;
    int region_w = 0;
    int region_h = 0;

    [[nodiscard]] bool has_subregion_geometry() const
    {
        return src_x != 0 || src_y != 0 || region_w != 0 || region_h != 0 || dest_x != 0 || dest_y != 0;
    }
};

class v210_output
{
  protected:
    v210_output() = default;

  public:
    v210_output& operator=(const v210_output&) = delete;
    virtual ~v210_output()                     = default;

    v210_output(const v210_output&) = delete;

    virtual void convert_frame(const core::video_format_desc& channel_format_desc,
                               const core::video_format_desc& output_format_desc,
                               const output_region&           region,
                               std::shared_ptr<void>          output,
                               const core::const_frame&       frame1,
                               const core::const_frame&       frame2,
                               uint8_t                        interlaced) = 0;
};

spl::shared_ptr<v210_output> create_v210_output(core::color_space colorspace, uint8_t bpc, bool straight_alpha);

}} // namespace caspar::v210
