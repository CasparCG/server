#include "../util/base64.hpp"
#include "vanc.h"

#if defined(_MSC_VER)
#else
#include <bits/stdc++.h>
#endif

#include <boost/lexical_cast.hpp>
#include <common/param.h>
#include <common/ptree.h>
#include <mutex>
#include <numeric>
#include <queue>
#include <vector>

namespace caspar { namespace decklink {

const uint8_t OP47_DID  = 0x43;
const uint8_t OP47_SDID = 0x02;

class vanc_op47_strategy : public decklink_vanc_strategy
{
  private:
    static const std::wstring Name;

    mutable std::mutex mutex_;
    uint8_t            line_number_;
    uint8_t            line_number_2_;
    uint8_t            sd_line_;
    uint16_t           counter_;

    std::vector<uint8_t>             dummy_header_;
    std::queue<std::vector<uint8_t>> packets_;

  public:
    vanc_op47_strategy(uint8_t line_number, uint8_t line_number_2, const std::wstring& dummy_header)
        : line_number_(line_number)
        , line_number_2_(line_number_2)
        , sd_line_(21)
        , counter_(1)
        , dummy_header_(dummy_header.empty() ? std::vector<uint8_t>() : base64_decode(dummy_header))
    {
    }

    virtual bool has_data() const override
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return !packets_.empty() || !dummy_header_.empty();
    }
    virtual vanc_packet pop_packet(bool field2) override
    {
        std::lock_guard<std::mutex> lock(mutex_);

        if (field2 && line_number_2_ == 0) {
            return {0, 0, 0, {}};
        }

        if (packets_.empty()) {
            return {OP47_DID, OP47_SDID, field2 ? line_number_2_ : line_number_, sdp_encode(dummy_header_)};
        }
        auto packet = packets_.front();
        packets_.pop();

        return {OP47_DID, OP47_SDID, field2 ? line_number_2_ : line_number_, packet};
    }

    virtual bool try_push_data(const std::vector<std::wstring>& params) override
    {
        std::lock_guard<std::mutex> lock(mutex_);

        try {
            for (size_t index = 1; index < params.size(); ++index) {
                packets_.push(sdp_encode(base64_decode(params.at(index))));
            }
        } catch (const boost::bad_lexical_cast& e) {
            CASPAR_LOG(error) << "Failed to parse OP47 parameters: " << e.what();
            return false;
        }

        return true;
    }

    virtual const std::wstring& get_name() const override { return vanc_op47_strategy::Name; }

  private:
    std::vector<uint8_t> sdp_encode(const std::vector<uint8_t>& packet)
    {
        if (packet.size() != 45) {
            throw std::runtime_error("Invalid packet size for OP47: " + std::to_string(packet.size()));
        }

        // The following is based on the specification "Free TV Australia Operational Practice OP- 47"

        std::vector<uint8_t> result(103);
        result[0] = 0x51;          // identifier
        result[1] = 0x15;          // identifier
        result[2] = static_cast<uint8_t>(result.size()); // size of the packet
        result[3] = 0x02;          // format-code

        result[4] = sd_line_ | 0xE0; // VBI packet descriptor (odd field)
        result[5] = sd_line_ | 0x60; // VBI packet descriptor (even field)
        result[6] = 0;               // VBI packet descriptor (not used)
        result[7] = 0;               // VBI packet descriptor (not used)
        result[8] = 0;               // VBI packet descriptor (not used)

        memcpy(result.data() + 9, packet.data(), packet.size());
        memcpy(result.data() + 9 + packet.size(), packet.data(), packet.size());
        result[99]  = 0x74;                     // footer id
        result[100] = (counter_ & 0xFF00) >> 8; // footer sequence counter
        result[101] = counter_ & 0x00FF;        // footer sequence counter
        result[102] = 0x0;                      // SPD checksum, will be set when calculated

        auto sum    = accumulate(result.begin(), result.end(), (uint8_t)0);
        result[102] = ~sum + 1;

        counter_++; // this is rolling over at 65535 by design

        return result;
    }

    std::vector<uint8_t> base64_decode(const std::wstring& encoded)
    {
        std::vector<char> buffer(encoded.size());
        std::use_facet<std::ctype<wchar_t>>(std::locale())
            .narrow(encoded.data(), encoded.data() + encoded.size(), '?', buffer.data());
        auto str = std::string(buffer.data(), buffer.size());
        return base64::decode_into<std::vector<uint8_t>>(str);
    }
};

const std::wstring vanc_op47_strategy::Name = L"OP47";

std::shared_ptr<decklink_vanc_strategy>
create_op47_strategy(uint8_t line_number, uint8_t line_number_2, const std::wstring& dummy_header)
{
    return std::make_shared<vanc_op47_strategy>(line_number, line_number_2, dummy_header);
}

}} // namespace caspar::decklink