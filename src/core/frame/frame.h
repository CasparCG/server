#pragma once

#include <common/array.h>

#include <any>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <future>
#include <memory>
#include <vector>

namespace caspar { namespace core {

class mutable_frame final
{
    friend class const_frame;

  public:
    using commit_t = std::function<std::any(std::vector<array<const std::uint8_t>>)>;

    explicit mutable_frame(const void*                      tag,
                           std::vector<array<std::uint8_t>> image_data,
                           array<std::int32_t>              audio_data,
                           const struct pixel_format_desc&  desc,
                           commit_t                         commit = nullptr);
    mutable_frame(const mutable_frame&) = delete;
    mutable_frame(mutable_frame&& other) noexcept;

    ~mutable_frame();

    mutable_frame& operator=(const mutable_frame&) = delete;
    mutable_frame& operator=(mutable_frame&& other);

    const struct pixel_format_desc& pixel_format_desc() const;

    array<std::uint8_t>&       image_data(std::size_t index);
    const array<std::uint8_t>& image_data(std::size_t index) const;

    array<std::int32_t>&       audio_data();
    const array<std::int32_t>& audio_data() const;

    std::size_t width() const;

    std::size_t height() const;

    class frame_geometry&       geometry();
    const class frame_geometry& geometry() const;

  private:
    struct impl;
    std::unique_ptr<impl> impl_;
};

class const_frame final
{
  public:
    const_frame();
    explicit const_frame(std::any image_ptr,
                         array<const std::int32_t>              audio_data,
                         const struct pixel_format_desc&        desc);
    const_frame(const const_frame& other);
    const_frame(mutable_frame&& other);

    ~const_frame();

    const_frame& operator=(const const_frame& other);

    const struct pixel_format_desc& pixel_format_desc() const;

    const std::any& image_ptr() const;

    const array<const std::int32_t>& audio_data() const;

    std::size_t width() const;

    std::size_t height() const;

    const class frame_geometry& geometry() const;

    bool operator==(const const_frame& other) const;
    bool operator!=(const const_frame& other) const;
    bool operator<(const const_frame& other) const;
    bool operator>(const const_frame& other) const;

    explicit operator bool() const;

  private:
    struct impl;
    std::shared_ptr<impl> impl_;
};

struct converted_frame
{
    const_frame                                   frame;
    std::shared_future<array<const std::uint8_t>> pixels;

    explicit converted_frame()
    {
    }

    converted_frame(const core::const_frame& frame, std::shared_future<array<const std::uint8_t>> pixels)
        : frame(frame)
        , pixels(std::move(pixels))
    {
    }

    bool is_empty() {
        return frame == core::const_frame{} || !pixels.valid();
    }
};

}} // namespace caspar::core
