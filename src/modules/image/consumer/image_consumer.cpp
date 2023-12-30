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
 * Author: Robert Nagy, ronag89@gmail.com
 */

#include "image_consumer.h"

#if defined(_MSC_VER)
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#endif
#include <FreeImage.h>

#include <common/array.h>
#include <common/env.h>
#include <common/future.h>
#include <common/log.h>
#include <common/param.h>
#include <common/utf.h>

#include <core/consumer/frame_consumer.h>
#include <core/frame/frame.h>
#include <core/frame/frame_factory.h>
#include <core/video_format.h>

#include <boost/algorithm/string.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

#include <utility>
#include <vector>

#include "../util/image_view.h"
#include "image/util/image_algorithms.h"

namespace caspar { namespace image {

#if FREEIMAGE_COLORORDER == FREEIMAGE_COLORORDER_BGR
#define IMAGE_ENCODED_FORMAT core::encoded_frame_format::bgra16
#else
#define IMAGE_ENCODED_FORMAT core::encoded_frame_format::rgba16
#endif

struct image_consumer : public core::frame_consumer
{
    const spl::shared_ptr<core::frame_converter> frame_converter_;
    const std::wstring                           filename_;
    const bool                                   depth16_;

  public:
    // frame_consumer

    explicit image_consumer(const spl::shared_ptr<core::frame_converter>& frame_converter,
                            std::wstring                                  filename,
                            bool                                          depth16)
        : frame_converter_(frame_converter)
        , filename_(std::move(filename))
        , depth16_(depth16)
    {
    }

    void initialize(const core::video_format_desc& /*format_desc*/, int /*channel_index*/) override {}

    std::future<bool> send(core::video_field field, core::const_frame frame) override
    {
        std::thread async([frame_converter = frame_converter_, depth16 = depth16_, frame, filename = filename_] {
            try {
                auto filename2 = filename;

                if (filename2.empty())
                    filename2 = env::media_folder() +
                                boost::posix_time::to_iso_wstring(boost::posix_time::second_clock::local_time()) +
                                L".png";
                else
                    filename2 = env::media_folder() + filename2 + L".png";

                std::shared_ptr<FIBITMAP> bitmap;
                common::bit_depth         frame_depth = frame_converter->get_frame_bitdepth(frame);

                if (depth16 && frame_depth != common::bit_depth::bit8) {
                    bitmap = std::shared_ptr<FIBITMAP>(FreeImage_AllocateT(FIT_RGBA16,
                                                                           static_cast<int>(frame.width()),
                                                                           static_cast<int>(frame.height())),
                                                       FreeImage_Unload);

                    array<const std::uint8_t> rgba16_bytes =
                        frame_converter->convert_from_rgba(frame, IMAGE_ENCODED_FORMAT, false, true).get();

                    std::memcpy(FreeImage_GetBits(bitmap.get()), rgba16_bytes.data(), rgba16_bytes.size());

                    // Note: premultiplication is done on the gpu
                } else {
                    bitmap = std::shared_ptr<FIBITMAP>(
                        FreeImage_AllocateT(
                            FIT_BITMAP, static_cast<int>(frame.width()), static_cast<int>(frame.height()), 32),
                        FreeImage_Unload);

                    std::memcpy(
                        FreeImage_GetBits(bitmap.get()), frame.image_data(0).begin(), frame.image_data(0).size());

                    image_view<bgra_pixel> original_view(FreeImage_GetBits(bitmap.get()),
                                                         static_cast<int>(frame.width()),
                                                         static_cast<int>(frame.height()));
                    unmultiply(original_view);
                }

                FreeImage_FlipVertical(bitmap.get());
#ifdef WIN32
                FreeImage_SaveU(FIF_PNG, bitmap.get(), filename2.c_str(), 0);
#else
                FreeImage_Save(FIF_PNG, bitmap.get(), u8(filename2).c_str(), 0);
#endif
            } catch (...) {
                CASPAR_LOG_CURRENT_EXCEPTION()
            }
        });
        async.detach();

        return make_ready_future(false);
    }

    std::wstring print() const override { return L"image[]"; }

    std::wstring name() const override { return L"image"; }

    int index() const override { return 100; }

    core::monitor::state state() const override
    {
        core::monitor::state state;
        state["image/filename"] = u8(filename_);
        return state;
    }
};

spl::shared_ptr<core::frame_consumer> create_consumer(const std::vector<std::wstring>&              params,
                                                      const core::video_format_repository&          format_repository,
                                                      const spl::shared_ptr<core::frame_converter>& frame_converter,
                                                      const std::vector<spl::shared_ptr<core::video_channel>>& channels)
{
    if (params.empty() || !boost::iequals(params.at(0), L"IMAGE"))
        return core::frame_consumer::empty();

    std::wstring filename;
    bool         depth16 = false;

    if (params.size() > 1)
        filename = params.at(1);
    if (params.size() > 2) {
        depth16 =
            contains_param(L"16BIT", params) || contains_param(L"16-BIT", params) || contains_param(L"16_BIT", params);
    }

    return spl::make_shared<image_consumer>(frame_converter, filename, depth16);
}

}} // namespace caspar::image
