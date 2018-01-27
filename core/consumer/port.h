#pragma once

#include "../monitor/monitor.h"
#include "../fwd.h"

#include <common/memory.h>

#include <boost/property_tree/ptree_fwd.hpp>

#include <future>

namespace caspar { namespace core {

class port
{
	port(const port&);
	port& operator=(const port&);
public:

	// Static Members

	// Constructors

	port(int index, int channel_index, spl::shared_ptr<frame_consumer> consumer);
	port(port&& other);
	~port();

	// Member Functions

	port& operator=(port&& other);

	std::future<bool> send(const_frame frame);

	monitor::subject& monitor_output();

	// Properties

	void change_channel_format(const video_format_desc& format_desc, const audio_channel_layout& channel_layout);
	std::wstring print() const;
	int buffer_depth() const;
	bool has_synchronization_clock() const;
	boost::property_tree::wptree info() const;
	int64_t presentation_frame_age_millis() const;
	spl::shared_ptr<const frame_consumer> consumer() const;
private:
	struct impl;
	std::unique_ptr<impl> impl_;
};

}}
