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
 * Author: Nicklas P Andersson
 */

#pragma once

#include <core/video_channel.h>

#include "../util/ProtocolStrategy.h"
#include "CIICommand.h"

#include <core/producer/cg_proxy.h>
#include <core/producer/stage.h>

#include <common/executor.h>
#include <common/memory.h>

#include <string>

namespace caspar { namespace protocol { namespace cii {

class CIIProtocolStrategy : public IO::IProtocolStrategy
{
  public:
    CIIProtocolStrategy(const std::vector<spl::shared_ptr<core::video_channel>>&    channels,
                        const spl::shared_ptr<core::cg_producer_registry>&          cg_registry,
                        const spl::shared_ptr<const core::frame_producer_registry>& producer_registry);

    void        Parse(const std::wstring& message, IO::ClientInfoPtr pClientInfo) override;
    std::string GetCodepage() const override { return "ISO-8859-1"; } // ISO 8859-1

    void SetProfile(const std::wstring& profile) { currentProfile_ = profile; }

    spl::shared_ptr<core::video_channel>                 GetChannel() const { return pChannel_; }
    spl::shared_ptr<core::cg_producer_registry>          get_cg_registry() const { return cg_registry_; }
    spl::shared_ptr<const core::frame_producer_registry> get_producer_registry() const { return producer_registry_; }
    core::frame_producer_dependencies                    get_dependencies() const
    {
        return core::frame_producer_dependencies(GetChannel()->frame_factory(),
                                                 channels_,
                                                 GetChannel()->video_format_desc(),
                                                 producer_registry_,
                                                 cg_registry_);
    }

    void DisplayMediaFile(const std::wstring& filename);
    void DisplayTemplate(const std::wstring& titleName);
    void
    WriteTemplateData(const std::wstring& templateName, const std::wstring& titleName, const std::wstring& xmlData);

  public:
    struct TitleHolder
    {
        TitleHolder()
            : titleName(L"")
            , pframe_producer(core::frame_producer::empty())
        {
        }
        TitleHolder(const std::wstring& name, spl::shared_ptr<core::frame_producer> pFP)
            : titleName(name)
            , pframe_producer(pFP)
        {
        }
        TitleHolder(const TitleHolder& th)
            : titleName(th.titleName)
            , pframe_producer(th.pframe_producer)
        {
        }
        TitleHolder& operator=(const TitleHolder& th)
        {
            titleName       = th.titleName;
            pframe_producer = th.pframe_producer;

            return *this;
        }
        bool operator==(const TitleHolder& rhs) const { return pframe_producer == rhs.pframe_producer; }

        std::wstring                          titleName;
        spl::shared_ptr<core::frame_producer> pframe_producer;
        friend CIIProtocolStrategy;
    };

  private:
    using TitleList = std::list<TitleHolder>;
    TitleList                             titles_;
    spl::shared_ptr<core::frame_producer> GetPreparedTemplate(const std::wstring& name);
    void PutPreparedTemplate(const std::wstring& name, const spl::shared_ptr<core::frame_producer>& pframe_producer);

    static const wchar_t      TokenDelimiter;
    static const std::wstring MessageDelimiter;

    void          ProcessMessage(const std::wstring& message, IO::ClientInfoPtr pClientInfo);
    int           TokenizeMessage(const std::wstring& message, std::vector<std::wstring>* pTokenVector);
    CIICommandPtr Create(const std::wstring& name);

    executor     executor_;
    std::wstring currentMessage_;

    std::wstring                                         currentProfile_;
    spl::shared_ptr<core::video_channel>                 pChannel_;
    spl::shared_ptr<core::cg_producer_registry>          cg_registry_;
    spl::shared_ptr<const core::frame_producer_registry> producer_registry_;
    std::vector<spl::shared_ptr<core::video_channel>>    channels_;
};

}}} // namespace caspar::protocol::cii
