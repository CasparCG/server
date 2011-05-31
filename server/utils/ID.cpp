#include "stdafx.h"
#include "ID.h"

namespace caspar
{
	namespace utils
	{

		ID::ID() : value_(0){}

		const ID::value_type& ID::Value() const
		{
			return value_;
		}
		ID ID::Generate(void* ptr)
		{			
			assert(sizeof(value_type) >= sizeof(LARGE_INTEGER));
			ID id;
			QueryPerformanceCounter(reinterpret_cast<LARGE_INTEGER*>(&id.value_));	
			id.value_ <<= 32;
			id.value_ |= reinterpret_cast<int>(ptr);
			return id;
		}

		bool operator ==(const ID& lhs,const ID& rhs)
		{
			return lhs.Value() == rhs.Value();
		}

		Identifiable::Identifiable() : id_(utils::ID::Generate(this))
		{}

		const utils::ID& Identifiable::ID() const
		{
			return id_;
		}
	}
}