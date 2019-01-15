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

#include "CIICommand.h"

namespace caspar { namespace protocol { namespace cii {

class CIIProtocolStrategy;

class MediaCommand : public ICIICommand
{
  public:
    MediaCommand(CIIProtocolStrategy* pPS)
        : pCIIStrategy_(pPS)
    {
    }

    int GetMinimumParameters() override { return 1; }

    void Setup(const std::vector<std::wstring>& parameters) override;
    void Execute() override;

  private:
    std::wstring graphicProfile_;

    CIIProtocolStrategy* pCIIStrategy_;
};

class WriteCommand : public ICIICommand
{
  public:
    WriteCommand(CIIProtocolStrategy* pPS)
        : pCIIStrategy_(pPS)
    {
    }

    int GetMinimumParameters() override { return 2; }

    void Setup(const std::vector<std::wstring>& parameters) override;
    void Execute() override;

  private:
    std::wstring targetName_;
    std::wstring templateName_;
    std::wstring xmlData_;

    CIIProtocolStrategy* pCIIStrategy_;
};

class MiscellaneousCommand : public ICIICommand
{
  public:
    MiscellaneousCommand(CIIProtocolStrategy* pPS)
        : state_(-1)
        , layer_(0)
        , pCIIStrategy_(pPS)
    {
    }

    int GetMinimumParameters() override { return 5; }

    void Setup(const std::vector<std::wstring>& parameters) override;
    void Execute() override;

  private:
    std::wstring filename_;
    std::wstring xmlData_;
    int          state_;
    int          layer_;

    CIIProtocolStrategy* pCIIStrategy_;
};

class ImagestoreCommand : public ICIICommand
{
  public:
    ImagestoreCommand(CIIProtocolStrategy* pPS)
        : pCIIStrategy_(pPS)
    {
    }

    int GetMinimumParameters() override { return 1; }

    void Setup(const std::vector<std::wstring>& parameters) override;
    void Execute() override;

  private:
    std::wstring titleName_;

    CIIProtocolStrategy* pCIIStrategy_;
};

class KeydataCommand : public ICIICommand
{
  public:
    KeydataCommand(CIIProtocolStrategy* pPS)
        : state_(-1)
        , layer_(0)
        , casparLayer_(0)
        , pCIIStrategy_(pPS)
    {
    }

    int GetMinimumParameters() override { return 1; }

    void Setup(const std::vector<std::wstring>& parameters) override;
    void Execute() override;

  private:
    std::wstring titleName_;
    int          state_;
    int          layer_;
    int          casparLayer_;

    CIIProtocolStrategy* pCIIStrategy_;
};

}}} // namespace caspar::protocol::cii
