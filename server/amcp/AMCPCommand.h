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