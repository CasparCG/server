#pragma once

#include <common/memory.h>

#include <core/monitor/monitor.h>

#include <functional>

namespace caspar { namespace protocol { namespace osc {

class server : public reactive::observer<monitor::event>
{
public:	
	server(unsigned short port);
	
	void on_next(const monitor::event& e) override;
private:
	spl::shared_ptr<observer<monitor::event>> impl_;
};

}}}