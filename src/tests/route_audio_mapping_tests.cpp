#include <core/producer/route/route_audio_mapping.h>

#include <array>
#include <stdexcept>
#include <vector>

namespace {

void check(bool condition)
{
    if (!condition) {
        throw std::runtime_error("route audio mapping sample test failed");
    }
}

} // namespace

int main()
{
    using caspar::core::parse_route_audio_mapping;
    using caspar::core::remap_route_audio;

    const auto                         mapping = parse_route_audio_mapping(L"2=0, 3=1, 4=0", 8, 8);
    const std::array<std::int32_t, 16> input   = {10, 11, 12, 13, 14, 15, 16, 17, 20, 21, 22, 23, 24, 25, 26, 27};
    const auto                         output  = remap_route_audio(input.data(), input.size(), 8, 8, mapping);

    check(output.size() == input.size());
    check(output[0] == 0 && output[1] == 0);
    check(output[2] == 10 && output[3] == 11);
    check(output[4] == 10 && output[5] == 0);
    check(output[10] == 20 && output[11] == 21);
    check(input[0] == 10 && input[1] == 11);

    const auto invalid_update = [&] { parse_route_audio_mapping(L"2=0,2=1", 8, 8); };
    bool       rejected       = false;
    try {
        invalid_update();
    } catch (const std::invalid_argument&) {
        rejected = true;
    }
    check(rejected);

    for (const auto& value :
         {L"", L"0", L"0==1", L"-1=0", L"0=-1", L"0=8", L"0=0,", L",0=0", L"0=0,,1=1", L"999999999999999999999=0"}) {
        rejected = false;
        try {
            parse_route_audio_mapping(value, 8, 8);
        } catch (const std::invalid_argument&) {
            rejected = true;
        }
        check(rejected);
    }

    rejected = false;
    try {
        parse_route_audio_mapping(L"8=0", 8, 8);
    } catch (const std::invalid_argument&) {
        rejected = true;
    }
    check(rejected);

    rejected = false;
    try {
        parse_route_audio_mapping(L"2=x", 8, 8);
    } catch (const std::invalid_argument&) {
        rejected = true;
    }
    check(rejected);

    const auto recovery_mapping = parse_route_audio_mapping(L"2=4,3=5", 8, 8);
    const auto recovered        = remap_route_audio(input.data(), input.size(), 8, 8, recovery_mapping);
    check(recovered[2] == 14 && recovered[3] == 15);
    check(recovered[10] == 24 && recovered[11] == 25);

    const std::array<std::int32_t, 3> partial_input   = {30, 31, 32};
    const auto                        partial_mapping = parse_route_audio_mapping(L"0=0,1=1", 2, 4);
    const auto partial = remap_route_audio(partial_input.data(), partial_input.size(), 2, 4, partial_mapping);
    check(partial.size() == 8);
    check(partial[0] == 30 && partial[1] == 31 && partial[2] == 0 && partial[3] == 0);
    check(partial[4] == 32 && partial[5] == 0);
}
