#ifndef _OBJECTPOOL_H_
#define _OBJECTPOOL_H_

#include <functional>
#include <memory>
#include <type_traits>

#include <tbb/concurrent_queue.h>

namespace caspar
{

namespace utils
{

template<typename T>
struct default_new_delete_allocator
{	
	static T* construct()
	{ return new T(); }

	
	template<typename P0>
	static T* construct(P0&& p0)
	{ return new T(std::forward<P0>(p0)); }

	template<typename P0, typename P1>
	static T* construct(P0&& p0, P1&& p1)
	{ return new T(std::forward<P0>(p0), std::forward<P1>(p1)); }
	
	template<typename P0, typename P1, typename P2>
	static T* construct(P0&& p0, P1&& p1, P2&& p2)
	{ return new T(std::forward<P0>(p0), std::forward<P1>(p1), std::forward<P2>(p2)); }

	static void destroy(T* const block)
	{ delete block; }
};

template<typename T, typename allocator = default_new_delete_allocator<T>>
class object_pool
{
	typedef std::shared_ptr<T> Ptr;
public:

	~object_pool()
	{
		T* item;
		while(pool_.try_pop(item))
			allocator::destroy(item);
	}

	Ptr construct()
	{
		T* item = pool_.try_pop(item) ? item : allocator::construct();
		return Ptr(item, [&](T* item){ pool_.push(item); });
	}
	
	template<typename P0>
	Ptr construct(P0&& p0)
	{
		T* item = pool_.try_pop(item) ? item : allocator::construct(std::forward<P0>(p0));
		return Ptr(item, [&](T* item){ pool_.push(item); });
	}
	
	template<typename P0, typename P1>
	Ptr construct(P0&& p0, P1&& p1)
	{
		T* item = pool_.try_pop(item) ? item : allocator::construct(std::forward<P0>(p0), std::forward<P1>(p1));
		return Ptr(item, [&](T* item){ pool_.push(item); });
	}
	
	template<typename P0, typename P1, typename P2>
	Ptr construct(P0&& p0, P1&& p1, P1&& p2)
	{
		T* item = pool_.try_pop(item) ? item : allocator::construct(std::forward<P0>(p0), std::forward<P1>(p1), std::forward<P2>(p2));
		return Ptr(item, [&](T* item){ pool_.push(item); });
	}
private:
	tbb::concurrent_queue<T*> pool_;
};

}}

#endif