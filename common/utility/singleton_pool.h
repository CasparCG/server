#pragma once
	   
#include <tbb/concurrent_queue.h>
#include <tbb/scalable_allocator.h>

namespace caspar {

template <typename T>
class singleton_pool
{
public:
	
	static T* allocate()
	{
		T* ptr;
		if(!get_pool().try_pop(ptr))
			ptr = static_cast<T*>(scalable_malloc(sizeof(T)));
		return ptr;
	}
	
	static void deallocate(T* ptr)
	{
		if(!get_pool().try_push(ptr))
			scalable_free(ptr);
	}

	static void destroy(T* ptr)
	{
		ptr->~T();
		deallocate(ptr);
	}

	static std::shared_ptr<T> make_shared()
	{
		return std::shared_ptr<T>(new(allocate()) T(), destroy);
	}
	
	template<typename P0>
	static std::shared_ptr<T> make_shared(P0 p0)
	{
		return std::shared_ptr<T>(new(allocate()) T(std::forward<P0>(p0)), destroy);
	}
	
	template<typename P0, typename P1>
	static std::shared_ptr<T> make_shared(P0 p0, P1 p1)
	{
		return std::shared_ptr<T>(new(allocate()) T(std::forward<P0>(p0), std::forward<P1>(p1)), destroy);
	}
	
	template<typename P0, typename P1, typename P2>
	static std::shared_ptr<T> make_shared(P0 p0, P1 p1, P2 p2)
	{
		return std::shared_ptr<T>(new(allocate()) T(std::forward<P0>(p0), std::forward<P1>(p1), std::forward<P2>(p2)), destroy);
	}
	
	template<typename P0, typename P1, typename P2, typename P3>
	static std::shared_ptr<T> make_shared(P0 p0, P1 p1, P2 p2, P3 p3)
	{
		return std::shared_ptr<T>(new(allocate()) T(std::forward<P0>(p0), std::forward<P1>(p1), std::forward<P2>(p2), std::forward<P3>(p3)), destroy);
	}

private:
	struct pool
	{
		~pool()
		{
			T* ptr;
			while(value.try_pop(ptr))
				scalable_free(ptr);
		}
		tbb::concurrent_bounded_queue<T*> value;
	};

	static tbb::concurrent_bounded_queue<T*>& get_pool()
	{
		static pool global_pool;
		return global_pool.value;
	}
};

}