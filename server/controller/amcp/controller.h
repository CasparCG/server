#pragma once

#include "../../renderer/renderer_fwd.h"

#include "exception.h"

#include "command.h"

#include <boost/thread.hpp>

#include <boost/signals2/connection.hpp>
#include <boost/algorithm/string/iter_find.hpp>
#include <boost/algorithm/string/finder.hpp>
#include <boost/assign.hpp>

#include <boost/fusion/algorithm/query/any.hpp>

#include <tbb/concurrent_queue.h>

#include <memory>
#include <string>
#include <vector>
#include <sstream>

//400 ERROR				Kommandot kunde inte förstås  
//401 [kommando] ERROR	Ogiltig kanal  
//402 [kommando] ERROR	Parameter saknas  
//403 [kommando] ERROR	Ogiltig parameter  
//404 [kommando] ERROR	Mediafilen hittades inte  
//500 FAILED				Internt serverfel  
//
//600 [kommando] FAILED	funktion ej implementerad
//501 [kommando] FAILED	Internt serverfel  
//502 [kommando] FAILED	Oläslig mediafil  

namespace caspar { namespace controller { namespace amcp {
			
template<typename message_stream>
class controller
{			
public:
	controller(message_stream& stream, const std::vector<renderer::render_device_ptr>& channels)
		: channels_(channels), is_running_(), stream_(stream)
	{
		thread_ = boost::thread(std::bind(&controller::run, this));
		connection_ = stream.subscribe(std::bind(&controller::on_next, this, std::placeholders::_1, std::placeholders::_2));
	}

	~controller()
	{
		is_running_ = false;
		connection_.disconnect();
		command_queue_.clear();
		command_queue_.push([]{});
		thread_.join();
	}

private:

	struct command_executor
	{
		template <typename command>
		bool operator()(command)
		{
			auto func = command::parse(message, channels);
			if(func != nullptr)
				reply = func();
			return func != nullptr;
		}
		const std::wstring& message;
		const std::vector<renderer::render_device_ptr>& channels;
		std::wstring& reply;
	};

	void on_next(const std::wstring& message, int tag)
	{				
		command_queue_.push([=]
		{
			unparsed_ += message;
			std::vector<std::wstring> message_split;
			boost::iter_split(message_split, unparsed_, boost::algorithm::first_finder(L"\r\n"));
			unparsed_ = message_split.back();
			for(size_t n = 0; n < message_split.size()-1; ++n)
			{
				try
				{
					std::wstring reply;
					command_executor executor = {message_split[n], channels_, reply};
					CASPAR_LOG(info) << "[amcp_controller] Received: " << message_split[n];
					if(!boost::fusion::any(command_list(), executor))
						stream_.start_write("400 INVALID\r\n", tag);
					else
					{
						std::vector<std::wstring> reply_split;
						boost::iter_split(reply_split, reply, boost::algorithm::first_finder(L"\r\n"));

						if(reply_split.empty())
							stream_.start_write("202 OK\r\n", tag);
						else if(reply_split.size() == 1)
						{
							stream_.start_write("201 OK\r\n", tag);
							stream_.start_write(common::narrow(reply_split[0]) + "\r\n", tag);
						}
						else
						{
							stream_.start_write("200 OK\r\n", tag);
							for(size_t n = 0; n < reply_split.size(); ++n)
								stream_.start_write(common::narrow(reply_split[n]) + "\r\n", tag);
						}
					}
				}
				catch(invalid_channel&)
				{
					stream_.start_write("401 INVALID\r\n", tag);
					CASPAR_LOG_CURRENT_EXCEPTION();
				}
				catch(file_not_found&)
				{
					stream_.start_write("404 ERROR\r\n", tag);
					CASPAR_LOG_CURRENT_EXCEPTION();
				}
				catch(file_read_error&)
				{
					stream_.start_write("502 FAILED\r\n", tag);
					CASPAR_LOG_CURRENT_EXCEPTION();
				}
				catch(not_implemented&)
				{
					stream_.start_write("600 FAILED\r\n", tag);
					CASPAR_LOG_CURRENT_EXCEPTION();
				}
				catch(...)
				{
					stream_.start_write("501 FAILED\r\n", tag);
					CASPAR_LOG_CURRENT_EXCEPTION();
				}
			}
		});		
	}
	
	void run()
	{
		is_running_ = true;
		while(is_running_)
		{
			boost::function<void()> func;
			command_queue_.pop(func);		
			func();
		}
	}

	std::wstring unparsed_;

	boost::signals2::scoped_connection connection_;
	message_stream& stream_;

	boost::thread thread_;
	tbb::atomic<bool> is_running_;
	tbb::concurrent_bounded_queue<boost::function<void()>> command_queue_;
	std::wstring active_message_;
	const std::vector<renderer::render_device_ptr>& channels_;
};


}}}