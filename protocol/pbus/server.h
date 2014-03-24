#pragma once

#include <boost/optional.hpp>

#include <cstdint>
#include <functional>
#include <memory>
#include <string>

namespace caspar { namespace protocol { namespace pbus {

class server sealed 
{
	server(const server&);
	server& operator=(const server&);
public:

	// Static Members

	struct parity
	{
		enum type 
		{
			none,
			odd,
			even
		};
	};
	
	struct flow_control
	{
		enum type 
		{
			none,
			software,
			hardware
		};
	};

	struct stop_bits
	{
		enum type
		{
			one, 
			onepointfive, 
			two 
		};
	};
	
	// Constructors

	server(const std::string& port, 
		   unsigned device_mask,
		   std::function<void(const std::string&)> listener);

	server(server&& other);

	~server();

	// Methods

	server& operator=(server&& other);

	// Properties

	void set_baud_rate(unsigned int value);
	void set_flow_control(flow_control::type value);
	void set_parity(parity::type value);
	void set_stop_bits(stop_bits::type value);
	void set_character_size(unsigned int value);

private:
	struct impl;
	std::unique_ptr<impl> impl_;
};

}}}