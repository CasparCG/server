#include "../StdAfx.h"

#include "server.h"

#include <common/log/log.h>
#include <common/env.h>
#include <common/utility/assert.h>
#include <common/exception/exceptions.h>
#include <common/exception/win32_exception.h>

#include <boost/array.hpp>
#include <boost/asio.hpp>
#include <boost/assign.hpp>
#include <boost/foreach.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/regex.hpp>
#include <boost/thread.hpp>
#include <boost/algorithm/string/predicate.hpp>

#include <algorithm>
#include <fstream>
#include <sstream>
#include <vector>
#include <sstream>

namespace caspar { namespace protocol { namespace pbus {

struct server::impl
{
	_declspec(align(64)) boost::array<char, 4096>	input_buffer;
	int												device_mask;
	boost::asio::io_service							io;
    boost::asio::serial_port						serial_port;
	const std::function<void(const std::string&)>   listener;
	std::vector<char>								storage_buffer;
	boost::thread									thread;

	impl(const std::string&						 port,
		 int									 device_mask,
		 std::function<void(const std::string&)> listener)
		: device_mask(device_mask)
		, io()
		, serial_port(io, port)
		, listener(std::move(listener))
		, thread(boost::bind(&impl::run, this))
	{
		input_buffer.fill('\r');
		storage_buffer.resize(1024, '\r');
		storage_buffer.resize(0);

		read_next_command();

		CASPAR_LOG(info) << L"[PBUS] Initialized.";
	}

	~impl()
	{
		io.stop();
	}

	void run()
	{
		win32_exception::install_handler();

		try
		{				
			io.run();
		}
		catch(...)
		{
			CASPAR_LOG_CURRENT_EXCEPTION();
		}
	}

	void read_next_command()
	{
		boost::asio::async_read(serial_port, 
								boost::asio::buffer(input_buffer),
								boost::bind(&impl::completion_condition, this, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred),
								boost::bind(&impl::handler, this, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
	}

	bool completion_condition(const boost::system::error_code& error, 
							  std::size_t					   bytes_transferred)
	{
		CASPAR_VERIFY(bytes_transferred < input_buffer.size());

		const auto begin = boost::begin(input_buffer);
		const auto end   = boost::begin(input_buffer) + bytes_transferred;
		
		return error || std::find(begin, end, '\r') != end;
	}

	void handler(const boost::system::error_code& error,
				 std::size_t					  bytes_transferred)
	{		
		CASPAR_VERIFY(bytes_transferred < input_buffer.size());

		if(error)
			return;

		std::wstringstream data_str;
		for (unsigned n = 0; n < bytes_transferred; ++n)
			data_str << std::hex << input_buffer[n];

		CASPAR_LOG(trace) << "[PBUS] Received " << data_str.str();

		storage_buffer.insert(boost::end(storage_buffer), boost::begin(input_buffer), boost::begin(input_buffer) + bytes_transferred);
		storage_buffer.erase(std::remove_if(boost::begin(storage_buffer), boost::end(storage_buffer), [](char c) -> bool
		{
			return c == '\n' || c == ' ' || c == '\t';
		}), boost::end(storage_buffer));
		std::fill_n(boost::begin(input_buffer), bytes_transferred, '\r');
		
		static const std::array<bool (impl::*)(const std::vector<char>&), 6> commands = 
		{ 
			&impl::learn,
			&impl::recall,
			&impl::trigger,
			&impl::query,
			&impl::read,
			&impl::write
		};
		
		try
		{
			while(true)
			{
				auto end_it = std::find(boost::begin(storage_buffer), boost::end(storage_buffer), '\r');
				
				if(end_it == boost::end(storage_buffer))
					break;

				end_it += 1;

				std::vector<char> command_buffer(boost::begin(storage_buffer), end_it);

				storage_buffer.erase(boost::begin(storage_buffer), end_it);

				if(std::find_if(boost::begin(commands), 
								boost::end(commands),
								[&](bool (impl::*func)(const std::vector<char>&))
								{
									return (this->*func)(command_buffer);
								}) == boost::end(commands))
				{
					CASPAR_LOG(warning) << "[PBUS] Command not found for keyword: " << input_buffer[0];
				}

			}
		}
		catch(...)
		{
			CASPAR_LOG_CURRENT_EXCEPTION();
		}

		read_next_command();
	}

	template<typename T>
	unsigned int hex_string_to_decimal(const T& str)
	{
		std::stringstream ss;
		ss << std::hex << std::string(boost::begin(str), boost::end(str));

		unsigned int value;
		ss >> value;
		return value;	
	}

	bool learn(const std::vector<char>& command_buffer)
	{
		struct learn_command
		{
			char keyword;
			char device_selection[6];
			char register_index[3];
			char eof;
		};
		
		const auto& command = *reinterpret_cast<const learn_command*>(command_buffer.data());
		
		if(command.keyword != 'L')
			return false;
		
		const auto device_selection = hex_string_to_decimal(command.device_selection);

		if(command.eof != '\r')
		{
			CASPAR_LOG(debug) << "[PBUS] Invalid data for 'learn' command"
							  << ", 'device selection': 0x" << std::hex << device_selection
							  << ".";
			return true;
		}

		// TODO
		
		//CASPAR_LOG(info) << "[PBUS] Successfully executed 'learn' command.";
		
		CASPAR_LOG(warning) << "[PBUS] 'learn' command is not implemented.";

		return true;
	}

	bool recall(const std::vector<char>& command_buffer)
	{
		struct recall_command
		{
			char keyword;
			char device_selection[6];
			char register_index[3];
			char eof;
		};

		const auto& command = *reinterpret_cast<const recall_command*>(command_buffer.data());
		
		if(command.keyword != 'R')
			return false;
		
		const auto device_selections = hex_string_to_decimal(command.device_selection);
		const auto active_register_index = hex_string_to_decimal(command.register_index);

		if(command.eof != '\r')
		{
			CASPAR_LOG(debug) << "[PBUS] Invalid data for 'recall' command"
							  << ", 'device selections': 0x" << std::hex << device_selections
							  << ", 'active register index': 0x" << std::hex << active_register_index
							  << ".";
			return true;
		}

		if ((device_selections & device_mask) == 0)
			return true;

		CASPAR_LOG(trace) << "[PBUS] Executing 'recall' command"
			<< ", 'device selections': 0x" << std::hex << device_selections
			<< ", 'active register index': 0x" << std::hex << active_register_index;

		for (auto device_selection = 0; device_selections >> device_selection; ++device_selection)
		{
			if ((((device_selections & device_mask) >> device_selection) & 1) == 0)
				continue;

			const auto filename = [&]() -> std::string
			{
				std::stringstream filename_ss;
				filename_ss << std::setw(2) << std::setfill('0') << device_selection << "R" << std::setw(2) << std::setfill('0') << active_register_index;
				return filename_ss.str();
			}();

			try
			{
				execute_macro(filename);

				CASPAR_LOG(info) << "[PBUS] Successfully executed 'recall' command"
					<< ", 'device selection': 0x" << std::hex << device_selection
					<< ", 'macro': " << filename.c_str()
					<< ".";
			}
			catch (...)
			{
				CASPAR_LOG(info) << "[PBUS] Failed to execute 'recall' command"
					<< ", 'device selection': " << std::hex << device_selection
					<< ", 'macro': " << filename.c_str();

				CASPAR_LOG_CURRENT_EXCEPTION();
			}
		}
		
		return true;
	}

	bool trigger(const std::vector<char>& command_buffer)
	{
		struct trigger_command
		{
			char keyword;
			char device_selection[6];
			char device_function[1];
			char eof;
		};
		
		const auto& command = *reinterpret_cast<const trigger_command*>(command_buffer.data());
		
		if(command.keyword != 'T')
			return false;
		
		const auto device_selections = hex_string_to_decimal(command.device_selection);
		const auto device_function = hex_string_to_decimal(command.device_function);

		if(command.eof != '\r')
		{
			CASPAR_LOG(debug) << "[PBUS] Invalid data for 'trigger' command"
							  << ", 'device selections': 0x" << std::hex << device_selections
							  << ", 'device function': 0x" << std::hex << device_function
							  << ".";
			return true;
		}				
		
		if ((device_selections & device_mask) == 0)
			return true;

		CASPAR_LOG(trace) << "[PBUS] Executing 'trigger' command"
			<< ", 'device selections': " << std::hex << device_selections
			<< ", 'device function': 0x" << std::hex << device_function
			<< ".";

		for (auto device_selection = 0; device_selections >> device_selection; ++device_selection)
		{
			if ((((device_selections & device_mask) >> device_selection) & 1) == 0)
				continue;

			const auto filename = [&]() -> std::string
			{
				std::stringstream filename_ss;
				filename_ss << std::setw(2) << std::setfill('0') << device_selection << "T" << std::setw(2) << std::setfill('0') << device_function;
				return filename_ss.str();
			}();

			try
			{
				execute_macro(filename);

				CASPAR_LOG(info) << "[PBUS] Successfully executed 'trigger' command"
					<< ", 'device selection': 0x" << std::hex << device_selection
					<< ", 'macro': " << filename.c_str()
					<< ".";
			}
			catch (...)
			{

				CASPAR_LOG(info) << "[PBUS] Failed to execute 'trigger' command"
					<< ", 'device selection': 0x" << std::hex << device_selection
					<< ", 'macro': " << filename.c_str()
					<< ".";

				CASPAR_LOG_CURRENT_EXCEPTION();
			}
		}

		return true;
	}

	bool query(const std::vector<char>& command_buffer)
	{		
		struct query_command
		{
			char keyword;
			char device_number[2];
			char eof;
		};
		
		const auto& command = *reinterpret_cast<const query_command*>(command_buffer.data());
		
		if(command.keyword != 'Q')
			return false;
				
		if(command.eof != '\r')
		{
			CASPAR_LOG(debug) << "[PBUS] Invalid data for 'query' command.";
			return true;
		}

		CASPAR_LOG(warning) << "[PBUS] 'query' command is not implemented.";

		return true;
	}
	
	bool read(const std::vector<char>& command_buffer)
	{
		struct read_command
		{
			char keyword;
			char device_number[2];
			char register_index[3];
			char eof;
		};
		
		const auto& command = *reinterpret_cast<const read_command*>(command_buffer.data());
		
		if(command.keyword != 'R')
			return false;
				
		if(command.eof != '\r')
		{
			CASPAR_LOG(debug) << "[PBUS] Invalid data for 'read' command.";
			return true;
		}

		CASPAR_LOG(warning) << "[PBUS] 'read' command is not implemented.";

		return true;
	}

	bool write(const std::vector<char>& command_buffer)
	{
		struct write_command
		{
			char keyword;
			char device_number[2];
			char register_index[3];
			char data[16];
		};
		
		const auto& command = *reinterpret_cast<const write_command*>(command_buffer.data());
		
		if(command.keyword != 'W')
			return false;
				
		if(std::find(boost::begin(command.data), boost::end(command.data), '\r') == boost::end(command.data))
		{
			CASPAR_LOG(debug) << "[PBUS] Invalid data for 'write' command.";
			return true;
		}

		CASPAR_LOG(warning) << "[PBUS] 'write' command is not implemented.";

		return true;
	}

	void execute_macro(std::string filename)
	{
		for (auto it = boost::filesystem::directory_iterator(narrow(env::macros_folder())); it != boost::filesystem::directory_iterator(); ++it)
		{
			if (boost::starts_with(it->filename(), filename))
			{
				filename = it->path().string();
				break;
			}
		}
	
		std::ifstream file(filename);

		if (!file.is_open())
			throw std::exception(("Could not open macro:" + filename).c_str());

		const auto lines = [&]() -> std::vector<std::string>
		{
			std::vector<std::string> lines;

			while (file.good())
			{
				std::string line;
				std::getline(file, line);
				lines.push_back(line);
			}

			file.close();

			return lines;
		}();

		CASPAR_LOG(info) << "[PBUS] Executing macro: " << widen(filename);

		BOOST_FOREACH(auto line, lines)
		{
			static boost::regex sleep_expr("(WAIT|SLEEP|DELAY) (?<MS>\\d+)", boost::regex::icase);
			static boost::regex single_line_comment_expr("//.*");

			line = boost::regex_replace(line, single_line_comment_expr, "");

			boost::smatch what;
			if (boost::regex_match(line, what, sleep_expr))
				boost::this_thread::sleep(boost::posix_time::milliseconds(boost::lexical_cast<int>(what["MS"].str())));
			else if (listener)
				listener(line);
		}
	}
};

server::server(const std::string&					   port,
			   unsigned								   device_mask,
		       std::function<void(const std::string&)> listener)
	: impl_(new impl(port,
					 device_mask,
					 listener))
{	
}

server::server(server&& other)
	: impl_(std::move(other.impl_))
{
}

server::~server()
{
}

server& server::operator=(server&& other)
{
	impl_ = std::move(other.impl_);
	return *this;
}

void server::set_baud_rate(unsigned int value)
{
	impl_->serial_port.set_option(boost::asio::serial_port_base::baud_rate(value));
}

void server::set_flow_control(flow_control::type value)
{
	if(value == flow_control::none)
		impl_->serial_port.set_option(boost::asio::serial_port_base::flow_control(boost::asio::serial_port_base::flow_control::none));
	else if(value == flow_control::software)
		impl_->serial_port.set_option(boost::asio::serial_port_base::flow_control(boost::asio::serial_port_base::flow_control::software));
	else if(value == flow_control::hardware)
		impl_->serial_port.set_option(boost::asio::serial_port_base::flow_control(boost::asio::serial_port_base::flow_control::hardware));
}

void server::set_parity(parity::type value)
{
	if(value == parity::none)
		impl_->serial_port.set_option(boost::asio::serial_port_base::parity(boost::asio::serial_port_base::parity::none));
	else if(value == parity::even)
		impl_->serial_port.set_option(boost::asio::serial_port_base::parity(boost::asio::serial_port_base::parity::even));
	else if(value == parity::odd)
		impl_->serial_port.set_option(boost::asio::serial_port_base::parity(boost::asio::serial_port_base::parity::odd));
}

void server::set_stop_bits(stop_bits::type value)
{
	if(value == stop_bits::one)
		impl_->serial_port.set_option(boost::asio::serial_port_base::stop_bits(boost::asio::serial_port_base::stop_bits::one));
	else if(value == stop_bits::onepointfive)
		impl_->serial_port.set_option(boost::asio::serial_port_base::stop_bits(boost::asio::serial_port_base::stop_bits::onepointfive));
	else if(value == stop_bits::two)
		impl_->serial_port.set_option(boost::asio::serial_port_base::stop_bits(boost::asio::serial_port_base::stop_bits::two));
}

void server::set_character_size(unsigned int value)
{
	impl_->serial_port.set_option(boost::asio::serial_port_base::character_size(value));
}


}}}