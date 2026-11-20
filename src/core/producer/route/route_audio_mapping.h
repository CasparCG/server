#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace caspar { namespace core {

inline std::wstring_view trim_route_audio_map_entry(std::wstring_view value)
{
    const auto first = value.find_first_not_of(L" \t\r\n");
    if (first == std::wstring_view::npos) {
        return {};
    }

    const auto last = value.find_last_not_of(L" \t\r\n");
    return value.substr(first, last - first + 1);
}

inline int parse_route_audio_map_index(std::wstring_view value)
{
    if (value.empty()) {
        throw std::invalid_argument("empty audio channel index");
    }

    int result = 0;
    for (const auto digit : value) {
        if (digit < L'0' || digit > L'9') {
            throw std::invalid_argument("audio channel indices must be non-negative integers");
        }

        const int number = digit - L'0';
        if (result > (std::numeric_limits<int>::max() - number) / 10) {
            throw std::invalid_argument("audio channel index is too large");
        }
        result = result * 10 + number;
    }
    return result;
}

// Each entry is destination=source. Unmapped destinations are represented by -1.
inline std::vector<int>
parse_route_audio_mapping(const std::wstring& value, int source_channels, int destination_channels)
{
    if (source_channels <= 0 || destination_channels <= 0) {
        throw std::invalid_argument("audio channel counts must be positive");
    }
    if (value.empty()) {
        throw std::invalid_argument("audio mapping cannot be empty");
    }

    std::vector<int> mapping(static_cast<std::size_t>(destination_channels), -1);
    std::size_t      entry_start = 0;
    while (entry_start < value.size()) {
        const auto comma  = value.find(L',', entry_start);
        auto       entry  = trim_route_audio_map_entry(std::wstring_view(value).substr(
            entry_start, comma == std::wstring::npos ? std::wstring::npos : comma - entry_start));
        const auto equals = entry.find(L'=');
        if (equals == std::wstring_view::npos || entry.find(L'=', equals + 1) != std::wstring_view::npos) {
            throw std::invalid_argument("each audio mapping entry must be destination=source");
        }

        const auto destination = parse_route_audio_map_index(trim_route_audio_map_entry(entry.substr(0, equals)));
        const auto source      = parse_route_audio_map_index(trim_route_audio_map_entry(entry.substr(equals + 1)));
        if (destination >= destination_channels || source >= source_channels) {
            throw std::invalid_argument("audio channel index is outside the source or destination channel count");
        }
        if (mapping[static_cast<std::size_t>(destination)] != -1) {
            throw std::invalid_argument("audio mapping contains a duplicate destination");
        }
        mapping[static_cast<std::size_t>(destination)] = source;

        if (comma == std::wstring::npos) {
            entry_start = value.size();
        } else {
            entry_start = comma + 1;
            if (entry_start == value.size()) {
                throw std::invalid_argument("audio mapping contains an empty entry");
            }
        }
    }
    return mapping;
}

inline std::vector<std::int32_t> remap_route_audio(const std::int32_t*     source,
                                                   std::size_t             source_size,
                                                   int                     source_channels,
                                                   int                     destination_channels,
                                                   const std::vector<int>& mapping)
{
    if (source_channels <= 0 || destination_channels <= 0 ||
        mapping.size() != static_cast<std::size_t>(destination_channels)) {
        throw std::invalid_argument("invalid route audio mapping dimensions");
    }

    const auto samples = source_size / static_cast<std::size_t>(source_channels) +
                         (source_size % static_cast<std::size_t>(source_channels) != 0 ? 1 : 0);
    if (samples > std::numeric_limits<std::size_t>::max() / static_cast<std::size_t>(destination_channels)) {
        throw std::length_error("route audio buffer is too large");
    }

    std::vector<std::int32_t> result(samples * static_cast<std::size_t>(destination_channels), 0);
    for (std::size_t sample = 0; sample < samples; ++sample) {
        for (int destination = 0; destination < destination_channels; ++destination) {
            const auto source_channel = mapping[static_cast<std::size_t>(destination)];
            if (source_channel < 0 || source_channel >= source_channels) {
                continue;
            }

            const auto source_index =
                sample * static_cast<std::size_t>(source_channels) + static_cast<std::size_t>(source_channel);
            if (source_index < source_size) {
                result[sample * static_cast<std::size_t>(destination_channels) +
                       static_cast<std::size_t>(destination)] = source[source_index];
            }
        }
    }
    return result;
}

}} // namespace caspar::core
