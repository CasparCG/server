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

#include "image_loader.h"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#if defined(_MSC_VER)
#include <windows.h>
#endif
#include <FreeImage.h>

#include <common/except.h>
#include <common/utf.h>

#if defined(_MSC_VER)
#pragma warning(disable : 4714) // marked as __forceinline not inlined
#endif

#include <boost/algorithm/string.hpp>
#include <boost/exception/errinfo_file_name.hpp>
#include <boost/filesystem.hpp>

#include "image_algorithms.h"
#include "image_view.h"

#if FREEIMAGE_COLORORDER == FREEIMAGE_COLORORDER_BGR
#define IMAGE_BGRA_FORMAT core::pixel_format::bgra
#define IMAGE_BGR_FORMAT core::pixel_format::bgr
#else
#define IMAGE_BGRA_FORMAT core::pixel_format::rgba
#define IMAGE_BGR_FORMAT core::pixel_format::rgb
#endif

namespace caspar { namespace image {

loaded_image prepare_loaded_image(FREE_IMAGE_FORMAT fif, std::shared_ptr<FIBITMAP> bitmap, bool allow_all_formats)
{
    core::pixel_format format;
    int                stride;
    common::bit_depth  depth = common::bit_depth::bit8;

    unsigned int bpp = FreeImage_GetBPP(bitmap.get());

    if (bpp == 32) {
        format = IMAGE_BGRA_FORMAT;
        stride = 4;
    } else if (allow_all_formats && bpp == 24) {
        format = IMAGE_BGR_FORMAT;
        stride = 3;
    } else if (allow_all_formats && bpp == 64) {
        // freeimage appears to ignore endianness
        format = core::pixel_format::rgba;
        stride = 4;
        depth  = common::bit_depth::bit16;
    } else if (allow_all_formats && bpp == 48) {
        // freeimage appears to ignore endianness
        format = core::pixel_format::rgb;
        stride = 3;
        depth  = common::bit_depth::bit16;
    } else if (allow_all_formats && !FreeImage_IsTransparent(bitmap.get())) {
        format = IMAGE_BGR_FORMAT;
        stride = 3;

        bitmap = std::shared_ptr<FIBITMAP>(FreeImage_ConvertTo24Bits(bitmap.get()), FreeImage_Unload);

    } else {
        format = IMAGE_BGRA_FORMAT;
        stride = 4;

        bitmap = std::shared_ptr<FIBITMAP>(FreeImage_ConvertTo32Bits(bitmap.get()), FreeImage_Unload);
    }

    if (!bitmap)
        CASPAR_THROW_EXCEPTION(invalid_argument() << msg_info("Unsupported image format."));

    // PNG-images need to be premultiplied with their alpha
    bool is_straight = fif == FIF_PNG && (format == core::pixel_format::bgra || format == core::pixel_format::rgba);
    if (!allow_all_formats && is_straight) {
        image_view<bgra_pixel> original_view(
            FreeImage_GetBits(bitmap.get()), FreeImage_GetWidth(bitmap.get()), FreeImage_GetHeight(bitmap.get()));
        premultiply(original_view);
    }

    return {std::move(bitmap), format, stride, depth, is_straight};
}

loaded_image load_image(const std::wstring& filename, bool allow_all_formats)
{
    if (!boost::filesystem::exists(filename))
        CASPAR_THROW_EXCEPTION(file_not_found() << boost::errinfo_file_name(u8(filename)));

#ifdef WIN32
    FREE_IMAGE_FORMAT fif = FreeImage_GetFileTypeU(filename.c_str(), 0);
#else
    FREE_IMAGE_FORMAT fif = FreeImage_GetFileType(u8(filename).c_str(), 0);
#endif

    if (fif == FIF_UNKNOWN)
#ifdef WIN32
        fif = FreeImage_GetFIFFromFilenameU(filename.c_str());
#else
        fif = FreeImage_GetFIFFromFilename(u8(filename).c_str());
#endif

    if (fif == FIF_UNKNOWN || (FreeImage_FIFSupportsReading(fif) == 0))
        CASPAR_THROW_EXCEPTION(invalid_argument() << msg_info("Unsupported image format."));

#ifdef WIN32
    auto bitmap = std::shared_ptr<FIBITMAP>(FreeImage_LoadU(fif, filename.c_str(), JPEG_EXIFROTATE), FreeImage_Unload);
#else
    auto bitmap =
        std::shared_ptr<FIBITMAP>(FreeImage_Load(fif, u8(filename).c_str(), JPEG_EXIFROTATE), FreeImage_Unload);
#endif

    return prepare_loaded_image(fif, std::move(bitmap), allow_all_formats);
}

loaded_image load_png_from_memory(const void* memory_location, size_t size, bool allow_all_formats)
{
    FREE_IMAGE_FORMAT fif = FIF_PNG;

    auto memory = std::unique_ptr<FIMEMORY, decltype(&FreeImage_CloseMemory)>(
        FreeImage_OpenMemory(static_cast<BYTE*>(const_cast<void*>(memory_location)), static_cast<DWORD>(size)),
        FreeImage_CloseMemory);
    auto bitmap =
        std::shared_ptr<FIBITMAP>(FreeImage_LoadFromMemory(fif, memory.get(), JPEG_EXIFROTATE), FreeImage_Unload);

    return prepare_loaded_image(fif, std::move(bitmap), allow_all_formats);
}

bool is_valid_file(const boost::filesystem::path& filename)
{
    static const std::set<std::wstring> extensions = {
        L".png", L".tga", L".bmp", L".jpg", L".jpeg", L".gif", L".tiff", L".tif", L".jp2", L".jpx", L".j2k", L".j2c"};

    auto ext = boost::to_lower_copy(boost::filesystem::path(filename).extension().wstring());
    if (extensions.find(ext) == extensions.end()) {
        return false;
    }

    return true;
}

}} // namespace caspar::image
