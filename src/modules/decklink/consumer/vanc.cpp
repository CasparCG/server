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
 * Author: Niklas Andersson, niklas.andersson@nxtedition.com
 */

#include "vanc.h"
#include <boost/lexical_cast.hpp>

namespace caspar { namespace decklink {

class decklink_vanc_packet : public IDeckLinkAncillaryPacket
{
    std::atomic<int> ref_count_{0};
    vanc_packet      pkt_;

  public:
    explicit decklink_vanc_packet(vanc_packet pkt)
        : pkt_(pkt)
    {
    }

    // IUnknown
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID, LPVOID*) override { return E_NOINTERFACE; }
    ULONG STDMETHODCALLTYPE   AddRef() override { return ++ref_count_; }
    ULONG STDMETHODCALLTYPE   Release() override
    {
        if (--ref_count_ == 0) {
            delete this;

            return 0;
        }

        return ref_count_;
    }

    // IDeckLinkAncillaryPacket
    HRESULT STDMETHODCALLTYPE GetBytes(BMDAncillaryPacketFormat format, const void** data, unsigned int* size) override
    {
        if (format == bmdAncillaryPacketFormatUInt8) {
            *data = pkt_.data.data();
            *size = static_cast<unsigned int>(pkt_.data.size());
            return S_OK;
        }

        return E_NOTIMPL;
    }
    unsigned char STDMETHODCALLTYPE GetDID(void) override { return pkt_.did; }
    unsigned char STDMETHODCALLTYPE GetSDID(void) override { return pkt_.sdid; }
    unsigned int STDMETHODCALLTYPE  GetLineNumber(void) override { return pkt_.line_number; }
    unsigned char STDMETHODCALLTYPE GetDataStreamIndex(void) override { return 0; }
};

decklink_vanc::decklink_vanc(const vanc_configuration& config)
{
    if (config.enable_scte104) {
        strategies_.push_back(create_scte104_strategy(config.scte104_line));
    }
}

std::vector<caspar::decklink::com_ptr<IDeckLinkAncillaryPacket>> decklink_vanc::pop_packets()
{
    std::vector<caspar::decklink::com_ptr<IDeckLinkAncillaryPacket>> packets;
    for (auto& strategy : strategies_) {
        if (strategy->has_data()) {
            try {
                packets.push_back(
                    wrap_raw<com_ptr, IDeckLinkAncillaryPacket>(new decklink_vanc_packet(strategy->pop_packet())));
            } catch (const std::exception& e) {
                CASPAR_LOG(error) << "Failed to pop " << strategy->get_name() << " VANC packet: " << e.what();
            } catch (...) {
                CASPAR_LOG(error) << "Failed to pop " << strategy->get_name() << " VANC packet.";
            }
        }
    }
    return packets;
}

bool decklink_vanc::has_data() const
{
    return std::any_of(strategies_.begin(), strategies_.end(), [](auto& strategy) { return strategy->has_data(); });
}

bool decklink_vanc::try_push_data(const std::vector<std::wstring>& params)
{
    if (params.size() < 2) {
        CASPAR_LOG(error) << " Not enough parameters to apply VANC data.";
        return false;
    }

    for (auto& strategy : strategies_) {
        if (boost::iequals(params.at(0), strategy->get_name())) {
            return strategy->try_push_data(params);
        }
    }
    return false;
}

std::shared_ptr<decklink_vanc> create_vanc(const vanc_configuration& config)
{
    return std::make_shared<decklink_vanc>(config);
}

}} // namespace caspar::decklink
