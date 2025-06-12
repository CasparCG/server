#include "../util/base64.hpp"
#include "vanc.h"

#include <mutex>

namespace caspar { namespace decklink {

const uint8_t SCTE104_DID  = 0x41;
const uint8_t SCTE104_SDID = 0x07;

class vanc_scte104_strategy : public decklink_vanc_strategy
{
    static const std::wstring Name;

    mutable std::mutex mutex_;
    uint8_t            line_number_;

    std::vector<uint8_t> payload_ = {};

  public:
    explicit vanc_scte104_strategy(uint8_t line_number)
        : line_number_(line_number)
    {
    }

    virtual bool has_data() const override
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return !payload_.empty();
    }
    virtual vanc_packet pop_packet() override
    {
        {
            std::lock_guard<std::mutex> lock(mutex_);

            // If we have a payload, return it as a vanc_packet.
            if (payload_.size() > 0) {
                auto        data = std::vector<uint8_t>(payload_.begin(), payload_.end());
                vanc_packet pkt{SCTE104_DID, SCTE104_SDID, line_number_, data};
                payload_.clear();
                return pkt;
            }
        }

        // If we have no data, return an empty vanc_packet.
        return {0, 0, 0, {}};
    }

    virtual bool try_push_data(const std::vector<std::wstring>& params) override
    {
        try {
            if (params.size() == 2) {
                std::lock_guard<std::mutex> lock(mutex_);

                // try to parse the payload as a base64 encoded raw SCTE-104 packet.
                auto base64_payload = params.at(1);
                payload_            = base64_decode(base64_payload);
            }
        } catch (const std::exception& e) {
            CASPAR_LOG(error) << "Failed to parse SCTE 104 parameters: " << e.what();
            return false;
        }

        return true;
    }

    virtual const std::wstring& get_name() const override { return vanc_scte104_strategy::Name; }

  private:
    std::vector<uint8_t> base64_decode(const std::wstring& encoded)
    {
        std::vector<char> buffer(encoded.size());
        std::use_facet<std::ctype<wchar_t>>(std::locale())
            .narrow(encoded.data(), encoded.data() + encoded.size(), '?', buffer.data());
        auto str = std::string(buffer.data(), buffer.size());
        return base64::decode_into<std::vector<uint8_t>>(str);
    }
};

const std::wstring vanc_scte104_strategy::Name = L"SCTE104";

std::shared_ptr<decklink_vanc_strategy> create_scte104_strategy(uint8_t line_number)
{
    return std::make_shared<vanc_scte104_strategy>(line_number);
}

}} // namespace caspar::decklink