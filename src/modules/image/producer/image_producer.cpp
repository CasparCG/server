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

#include "image_producer.h"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <FreeImage.h>

#include "../util/image_loader.h"

#include <core/video_format.h>

#include <core/frame/draw_frame.h>
#include <core/frame/frame.h>
#include <core/frame/frame_factory.h>
#include <core/frame/pixel_format.h>
#include <core/monitor/monitor.h>
#include <core/producer/frame_producer.h>

#include <common/array.h>
#include <common/base64.h>
#include <common/env.h>
#include <common/log.h>
#include <common/os/filesystem.h>
#include <common/param.h>

#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include <boost/property_tree/ptree.hpp>

#include <algorithm>
#include <set>

namespace caspar { namespace image {

struct image_producer : public core::frame_producer_base
{
    core::monitor::state                       state_;
    const std::wstring                         description_;
    const spl::shared_ptr<core::frame_factory> frame_factory_;
    const uint32_t                             length_;
    core::draw_frame                           frame_;

    image_producer(const spl::shared_ptr<core::frame_factory>& frame_factory,
                   const std::wstring&                         description,
                   uint32_t                                    length)
        : description_(description)
        , frame_factory_(frame_factory)
        , length_(length)
    {
        load(load_image(description_));

        CASPAR_LOG(info) << print() << L" Initialized";
    }

    image_producer(const spl::shared_ptr<core::frame_factory>& frame_factory,
                   const void*                                 png_data,
                   size_t                                      size,
                   uint32_t                                    length)
        : description_(L"png from memory")
        , frame_factory_(frame_factory)
        , length_(length)
    {
        load(load_png_from_memory(png_data, size));

        CASPAR_LOG(info) << print() << L" Initialized";
    }

    void load(const std::shared_ptr<FIBITMAP>& bitmap)
    {
        FreeImage_FlipVertical(bitmap.get());
        core::pixel_format_desc desc;
        desc.format = core::pixel_format::bgra;
        desc.planes.push_back(
            core::pixel_format_desc::plane(FreeImage_GetWidth(bitmap.get()), FreeImage_GetHeight(bitmap.get()), 4));
        auto frame = frame_factory_->create_frame(this, desc);

        std::copy_n(FreeImage_GetBits(bitmap.get()), frame.image_data(0).size(), frame.image_data(0).begin());
        frame_ = core::draw_frame(std::move(frame));
    }

    // frame_producer

    core::draw_frame receive_impl() override
    {
        state_["file/path"] = description_;

        return frame_;
    }

    uint32_t nb_frames() const override { return length_; }

    std::wstring print() const override { return L"image_producer[" + description_ + L"]"; }

    std::wstring name() const override { return L"image"; }

    const core::monitor::state& state() const { return state_; }
};

class ieq
{
    std::wstring test_;

  public:
    ieq(const std::wstring& test)
        : test_(test)
    {
    }

    bool operator()(const std::wstring& elem) const { return boost::iequals(elem, test_); }
};

spl::shared_ptr<core::frame_producer> create_producer(const core::frame_producer_dependencies& dependencies,
                                                      const std::vector<std::wstring>&         params)
{
    auto length = get_param(L"LENGTH", params, std::numeric_limits<uint32_t>::max());

    // if (boost::iequals(params.at(0), L"[IMG_SEQUENCE]"))
    //{
    //	if (params.size() != 2)
    //		return core::frame_producer::empty();

    //	auto dir = boost::filesystem::path(env::media_folder() + params.at(1)).parent_path();
    //	auto basename = boost::filesystem::basename(params.at(1));
    //	std::set<std::wstring> files;
    //	boost::filesystem::directory_iterator end;

    //	for (boost::filesystem::directory_iterator it(dir); it != end; ++it)
    //	{
    //		auto name = it->path().filename().wstring();

    //		if (!boost::algorithm::istarts_with(name, basename))
    //			continue;

    //		auto extension = it->path().extension().wstring();

    //		if (std::find_if(supported_extensions().begin(), supported_extensions().end(), ieq(extension)) ==
    // supported_extensions().end()) 			continue;

    //		files.insert(it->path().wstring());
    //	}

    //	if (files.empty())
    //		return core::frame_producer::empty();

    //	int width = -1;
    //	int height = -1;
    //	std::vector<core::draw_frame> frames;
    //	frames.reserve(files.size());

    //	for (auto& file : files)
    //	{
    //		auto frame = load_image(dependencies.frame_factory, file);

    //		if (width == -1)
    //		{
    //			width = static_cast<int>(frame.second.width.get());
    //			height = static_cast<int>(frame.second.height.get());
    //		}

    //		frames.push_back(std::move(frame.first));
    //	}

    //	return core::create_const_producer(std::move(frames), width, height);
    //}
    // else
    //if (boost::iequals(params.at(0), L"[PNG_BASE64]")) {
    //    if (params.size() < 2)
    //        return core::frame_producer::empty();

    //    auto png_data = from_base64(std::string(params.at(1).begin(), params.at(1).end()));

    //    return spl::make_shared<image_producer>(dependencies.frame_factory, png_data.data(), png_data.size(), length);
    //}

    std::wstring filename = env::media_folder() + params.at(0);

    auto ext = std::find_if(supported_extensions().begin(), supported_extensions().end(), [&](const std::wstring& ex) -> bool
    {
    	auto file = caspar::find_case_insensitive(boost::filesystem::path(filename).wstring() + ex);

    	return static_cast<bool>(file);
    });

    if (ext == supported_extensions().end()) {
        return core::frame_producer::empty();
    }

    return spl::make_shared<image_producer>(dependencies.frame_factory, *caspar::find_case_insensitive(filename + *ext), length);
}

}} // namespace caspar::image
