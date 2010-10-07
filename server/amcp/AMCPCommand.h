/*
* copyright (c) 2010 Sveriges Television AB <info@casparcg.com>
*
*  This file is part of CasparCG.
*
*    CasparCG is free software: you can redistribute it and/or modify
*    it under the terms of the GNU General Public License as published by
*    the Free Software Foundation, either version 3 of the License, or
*    (at your option) any later version.
*
*    CasparCG is distributed in the hope that it will be useful,
*    but WITHOUT ANY WARRANTY; without even the implied warranty of
*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*    GNU General Public License for more details.

*    You should have received a copy of the GNU General Public License
*    along with CasparCG.  If not, see <http://www.gnu.org/licenses/>.
*
*/
 
#ifndef __AMCPCOMMAND_H__
#define __AMCPCOMMAND_H__

#include "..\Channel.h"
#include "..\io\clientinfo.h"
#include <string>
#include <vector>

namespace caspar {
namespace amcp {

	enum AMCPCommandCondition {
		ConditionGood = 1,
		ConditionTemporarilyBad,
		ConditionPermanentlyBad
	};

	enum AMCPCommandScheduling
	{
		Default = 0,
		AddToQueue,
		ImmediatelyAndClear
	};

	class AMCPCommand
	{
		AMCPCommand(const AMCPCommand&);
		AMCPCommand& operator=(const AMCPCommand&);
	public:
		AMCPCommand();
		virtual ~AMCPCommand() {}
		virtual bool Execute() = 0;
		virtual AMCPCommandCondition CheckConditions() = 0;

		virtual bool NeedChannel() = 0;
		virtual AMCPCommandScheduling GetDefaultScheduling() = 0;
		virtual int GetMinimumParameters() = 0;

		void SendReply();

		void AddParameter(const tstring& param) {
			_parameters.push_back(param);
		}
		void SetClientInfo(caspar::IO::ClientInfoPtr& s) {
			pClientInfo_ = s;
		}
		caspar::IO::ClientInfoPtr GetClientInfo() {
			return pClientInfo_;
		}
		void SetChannel(const ChannelPtr& pChannel) {
			pChannel_ = pChannel;
		}
		ChannelPtr GetChannel() {
			return pChannel_;
		}
		void SetChannelIndex(unsigned int channelIndex) {
			channelIndex_ = channelIndex;
		}
		unsigned int GetChannelIndex() {
			return channelIndex_;
		}
		virtual void Clear();

		AMCPCommandScheduling GetScheduling()
		{
			return scheduling_ == Default ? GetDefaultScheduling() : scheduling_;
		}
		void SetScheduling(AMCPCommandScheduling s)
		{
			scheduling_ = s;
		}

	protected:
		void SetReplyString(const tstring& str) {
			replyString_ = str;
		}
		std::vector<tstring> _parameters;

	private:
		unsigned int channelIndex_;
		caspar::IO::ClientInfoPtr pClientInfo_;
		ChannelPtr pChannel_;
		AMCPCommandScheduling scheduling_;
		tstring replyString_;
	};

	typedef std::tr1::shared_ptr<AMCPCommand> AMCPCommandPtr;

}	//namespace amcp
}	//namespace caspar

#endif	//__AMCPCOMMAND_H__