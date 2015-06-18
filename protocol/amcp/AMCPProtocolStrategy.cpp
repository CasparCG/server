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

 
#include "../StdAfx.h"

#include "AMCPProtocolStrategy.h"
#include "AMCPCommandsImpl.h"
#include "amcp_shared.h"
#include "AMCPCommand.h"
#include "AMCPCommandQueue.h"

#include <stdio.h>
#include <string.h>
#include <algorithm>
#include <cctype>
#include <future>

#include <boost/algorithm/string/trim.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/lexical_cast.hpp>

#if defined(_MSC_VER)
#pragma warning (push, 1) // TODO: Legacy code, just disable warnings
#endif

namespace caspar { namespace protocol { namespace amcp {

using IO::ClientInfoPtr;

struct AMCPProtocolStrategy::impl
{
private:
	std::vector<channel_context>							channels_;
	std::vector<AMCPCommandQueue::ptr_type>					commandQueues_;
	std::shared_ptr<core::thumbnail_generator>				thumb_gen_;
	spl::shared_ptr<core::media_info_repository>			media_info_repo_;
	spl::shared_ptr<core::system_info_provider_repository>	system_info_provider_repo_;
	spl::shared_ptr<core::cg_producer_registry>				cg_registry_;
	std::promise<bool>&										shutdown_server_now_;

public:
	impl(
			const std::vector<spl::shared_ptr<core::video_channel>>& channels,
			const std::shared_ptr<core::thumbnail_generator>& thumb_gen,
			const spl::shared_ptr<core::media_info_repository>& media_info_repo,
			const spl::shared_ptr<core::system_info_provider_repository>& system_info_provider_repo,
			const spl::shared_ptr<core::cg_producer_registry>& cg_registry,
			std::promise<bool>& shutdown_server_now)
		: thumb_gen_(thumb_gen)
		, media_info_repo_(media_info_repo)
		, system_info_provider_repo_(system_info_provider_repo)
		, cg_registry_(cg_registry)
		, shutdown_server_now_(shutdown_server_now)
	{
		commandQueues_.push_back(std::make_shared<AMCPCommandQueue>());

		int index = 0;
		for (const auto& channel : channels)
		{
			std::wstring lifecycle_key = L"lock" + boost::lexical_cast<std::wstring>(index);
			channels_.push_back(channel_context(channel, lifecycle_key));
			auto queue(std::make_shared<AMCPCommandQueue>());
			commandQueues_.push_back(queue);
			++index;
		}
	}

	~impl() {}

	enum class parser_state {
		New = 0,
		GetSwitch,
		GetCommand,
		GetParameters
	};
	enum class error_state {
		no_error = 0,
		command_error,
		channel_error,
		parameters_error,
		unknown_error,
		access_error
	};

	struct command_interpreter_result
	{
		command_interpreter_result() : error(error_state::no_error) {}

		std::shared_ptr<caspar::IO::lock_container>	lock;
		std::wstring								command_name;
		AMCPCommand::ptr_type						command;
		error_state									error;
		AMCPCommandQueue::ptr_type					queue;
	};

	//The paser method expects message to be complete messages with the delimiter stripped away.
	//Thesefore the AMCPProtocolStrategy should be decorated with a delimiter_based_chunking_strategy
	void Parse(const std::wstring& message, ClientInfoPtr client)
	{
		CASPAR_LOG(info) << L"Received message from " << client->print() << ": " << message << L"\\r\\n";
	
		command_interpreter_result result;
		if(interpret_command_string(message, result, client))
		{
			if(result.lock && !result.lock->check_access(client))
				result.error = error_state::access_error;
			else
				result.queue->AddCommand(result.command);
		}
		
		if (result.error != error_state::no_error)
		{
			std::wstringstream answer;
			boost::to_upper(result.command_name);

			switch(result.error)
			{
			case error_state::command_error:
				answer << L"400 ERROR\r\n" << message << "\r\n";
				break;
			case error_state::channel_error:
				answer << L"401 " << result.command_name << " ERROR\r\n";
				break;
			case error_state::parameters_error:
				answer << L"402 " << result.command_name << " ERROR\r\n";
				break;
			case error_state::access_error:
				answer << L"503 " << result.command_name << " FAILED\r\n";
				break;
			default:
				answer << L"500 FAILED\r\n";
				break;
			}
			client->send(answer.str());
		}
	}

private:
	friend class AMCPCommand;

	bool interpret_command_string(const std::wstring& message, command_interpreter_result& result, ClientInfoPtr client)
	{
		try
		{
			std::vector<std::wstring> tokens;
			parser_state state = parser_state::New;

			tokenize(message, &tokens);

			//parse the message one token at the time
			auto end = tokens.end();
			auto it = tokens.begin();
			while (it != end && result.error == error_state::no_error)
			{
				switch(state)
				{
				case parser_state::New:
					if((*it)[0] == L'/')
						state = parser_state::GetSwitch;
					else
						state = parser_state::GetCommand;
					break;

				case parser_state::GetSwitch:
					//command_switch = (*it);	//we dont care for the switch anymore
					state = parser_state::GetCommand;
					++it;
					break;

				case parser_state::GetCommand:
					{
						result.command_name = (*it);
						result.command = create_command(result.command_name, client);
						if(result.command)	//the command doesn't need a channel
						{
							result.queue = commandQueues_[0];
							state = parser_state::GetParameters;
						}
						else
						{
							//get channel index from next token
							int channel_index = -1;
							int layer_index = -1;

							++it;
							if(it == end)
							{
								if(create_channel_command(result.command_name, client, channels_.at(0), 0, 0))	//check if there is a command like this
									result.error = error_state::channel_error;
								else
									result.error = error_state::command_error;

								break;
							}

							{	//parse channel/layer token
								try
								{
									std::wstring channelid_str = boost::trim_copy(*it);
									std::vector<std::wstring> split;
									boost::split(split, channelid_str, boost::is_any_of("-"));

									channel_index = boost::lexical_cast<int>(split[0]) - 1;
									if(split.size() > 1)
										layer_index = boost::lexical_cast<int>(split[1]);
								}
								catch(...)
								{
									result.error = error_state::channel_error;
									break;
								}
							}
						
							if(channel_index >= 0 && channel_index < channels_.size())
							{
								result.command = create_channel_command(result.command_name, client, channels_.at(channel_index), channel_index, layer_index);
								if(result.command)
								{
									result.lock = channels_.at(channel_index).lock;
									result.queue = commandQueues_[channel_index + 1];
								}
								else
								{
									result.error = error_state::command_error;
									break;
								}
							}
							else
							{
								result.error = error_state::channel_error;
								break;
							}
						}

						state = parser_state::GetParameters;
						++it;
					}
					break;

				case parser_state::GetParameters:
					{
						int parameterCount=0;
						while(it != end)
						{
							result.command->parameters().push_back((*it));
							++it;
							++parameterCount;
						}
					}
					break;
				}
			}

			if(result.command && result.error == error_state::no_error && result.command->parameters().size() < result.command->minimum_parameters()) {
				result.error = error_state::parameters_error;
			}
		}
		catch(...)
		{
			CASPAR_LOG_CURRENT_EXCEPTION();
			result.error = error_state::unknown_error;
		}

		return result.error == error_state::no_error;
	}

	std::size_t tokenize(const std::wstring& message, std::vector<std::wstring>* pTokenVector)
	{
		//split on whitespace but keep strings within quotationmarks
		//treat \ as the start of an escape-sequence: the following char will indicate what to actually put in the string

		std::wstring currentToken;

		bool inQuote = false;
		bool getSpecialCode = false;

		for(unsigned int charIndex=0; charIndex<message.size(); ++charIndex)
		{
			if(getSpecialCode)
			{
				//insert code-handling here
				switch(message[charIndex])
				{
				case L'\\':
					currentToken += L"\\";
					break;
				case L'\"':
					currentToken += L"\"";
					break;
				case L'n':
					currentToken += L"\n";
					break;
				default:
					break;
				};
				getSpecialCode = false;
				continue;
			}

			if(message[charIndex]==L'\\')
			{
				getSpecialCode = true;
				continue;
			}

			if(message[charIndex]==L' ' && inQuote==false)
			{
				if(currentToken.size()>0)
				{
					pTokenVector->push_back(currentToken);
					currentToken.clear();
				}
				continue;
			}

			if(message[charIndex]==L'\"')
			{
				inQuote = !inQuote;

				if(currentToken.size()>0 || !inQuote)
				{
					pTokenVector->push_back(currentToken);
					currentToken.clear();
				}
				continue;
			}

			currentToken += message[charIndex];
		}

		if(currentToken.size()>0)
		{
			pTokenVector->push_back(currentToken);
			currentToken.clear();
		}

		return pTokenVector->size();
	}

	AMCPCommand::ptr_type create_command(const std::wstring& str, ClientInfoPtr client)
	{
		std::wstring s = boost::to_upper_copy(str);
		if (     s == L"DIAG")			return std::make_shared<DiagnosticsCommand>(client);
		else if (s == L"CHANNEL_GRID")	return std::make_shared<ChannelGridCommand>(client, channels_);
		else if (s == L"DATA")			return std::make_shared<DataCommand>(client);
		else if (s == L"CINF")			return std::make_shared<CinfCommand>(client, media_info_repo_);
		else if (s == L"INFO")			return std::make_shared<InfoCommand>(client, channels_, system_info_provider_repo_, cg_registry_);
		else if (s == L"CLS")			return std::make_shared<ClsCommand>(client, media_info_repo_);
		else if (s == L"TLS")			return std::make_shared<TlsCommand>(client, cg_registry_);
		else if (s == L"VERSION")		return std::make_shared<VersionCommand>(client, system_info_provider_repo_);
		else if (s == L"BYE")			return std::make_shared<ByeCommand>(client);
		else if (s == L"LOCK")			return std::make_shared<LockCommand>(client, channels_);
		else if (s == L"LOG")			return std::make_shared<LogCommand>(client);
		else if (s == L"THUMBNAIL")		return std::make_shared<ThumbnailCommand>(client, thumb_gen_);
		else if (s == L"KILL")			return std::make_shared<KillCommand>(client, shutdown_server_now_);
		else if (s == L"RESTART")		return std::make_shared<RestartCommand>(client, shutdown_server_now_);

		return nullptr;
	}

	AMCPCommand::ptr_type create_channel_command(const std::wstring& str, ClientInfoPtr client, const channel_context& channel, unsigned int channel_index, int layer_index)
	{
		std::wstring s = boost::to_upper_copy(str);
	
		if (     s == L"MIXER") 	return std::make_shared<MixerCommand>(client, channel, channel_index, layer_index);
		else if (s == L"CALL")  	return std::make_shared<CallCommand>(client, channel, channel_index, layer_index);
		else if (s == L"SWAP")  	return std::make_shared<SwapCommand>(client, channel, channel_index, layer_index, channels_);
		else if (s == L"LOAD")  	return std::make_shared<LoadCommand>(client, channel, channel_index, layer_index, channels_);
		else if (s == L"LOADBG")	return std::make_shared<LoadbgCommand>(client, channel, channel_index, layer_index, channels_);
		else if (s == L"ADD")   	return std::make_shared<AddCommand>(client, channel, channel_index, layer_index);
		else if (s == L"REMOVE")	return std::make_shared<RemoveCommand>(client, channel, channel_index, layer_index);
		else if (s == L"PAUSE") 	return std::make_shared<PauseCommand>(client, channel, channel_index, layer_index);
		else if (s == L"PLAY")  	return std::make_shared<PlayCommand>(client, channel, channel_index, layer_index, channels_);
		else if (s == L"STOP")  	return std::make_shared<StopCommand>(client, channel, channel_index, layer_index);
		else if (s == L"CLEAR") 	return std::make_shared<ClearCommand>(client, channel, channel_index, layer_index);
		else if (s == L"PRINT") 	return std::make_shared<PrintCommand>(client, channel, channel_index, layer_index);
		else if (s == L"CG")	   	return std::make_shared<CGCommand>(client, channel, channel_index, layer_index, cg_registry_, channels_);
		else if (s == L"SET")   	return std::make_shared<SetCommand>(client, channel, channel_index, layer_index);

		return nullptr;
	}
};


AMCPProtocolStrategy::AMCPProtocolStrategy(
		const std::vector<spl::shared_ptr<core::video_channel>>& channels,
		const std::shared_ptr<core::thumbnail_generator>& thumb_gen,
		const spl::shared_ptr<core::media_info_repository>& media_info_repo,
		const spl::shared_ptr<core::system_info_provider_repository>& system_info_provider_repo,
		const spl::shared_ptr<core::cg_producer_registry>& cg_registry,
		std::promise<bool>& shutdown_server_now)
	: impl_(spl::make_unique<impl>(
			channels,
			thumb_gen,
			media_info_repo,
			system_info_provider_repo,
			cg_registry,
			shutdown_server_now))
{
}
AMCPProtocolStrategy::~AMCPProtocolStrategy() {}
void AMCPProtocolStrategy::Parse(const std::wstring& msg, IO::ClientInfoPtr pClientInfo) { impl_->Parse(msg, pClientInfo); }


}	//namespace amcp
}}	//namespace caspar
