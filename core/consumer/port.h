#pragma once

#include "../monitor/monitor.h"

#include <common/memory.h>

#include <boost/property_tree/ptree_fwd.hpp>
#include <boost/thread/future.hpp>

namespace caspar { namespace core {

class port : public monitor::observable
{
	port(const port&);
	port& operator=(const port&);
public:

	// Static Members

	// Constructors

	port(int index, int channel_index, spl::shared_ptr<class frame_consumer> consumer);
	port(port&& other);
	~port();

	// Member Functions

	port& operator=(port&& other);

	boost::unique_future<bool> send(class const_frame frame);	

	// monitor::observable
	
	void subscribe(const monitor::observable::observer_ptr& o) override;
	void unsubscribe(const monitor::observable::observer_ptr& o) override;

	// Properties

	void video_format_desc(const struct video_format_desc& format_desc);
	std::wstring print() const;
	int buffer_depth() const;
	bool has_synchronization_clock() const;
	boost::property_tree::wptree info() const;
private:
	struct impl;
	std::unique_ptr<impl> impl_;
};

}}