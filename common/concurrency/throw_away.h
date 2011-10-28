#pragma once

#include <agents.h>

namespace caspar {

template<typename T>
void link_throw_away(Concurrency::ISource<T>& source)
{
	static Concurrency::call<T> throw_away([](const T& elem){});
	source.link_target(&throw_away);
}

}