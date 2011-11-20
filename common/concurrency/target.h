#pragma once

namespace caspar {

template<typename T>
struct target
{
	virtual void send(const T&) = 0;
};

}