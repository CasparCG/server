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

template<typename T, typename Factory = std::function<T*()>, typename PoolType = tbb::concurrent_queue<T*>>
class ObjectPool
{
	static_assert(std::tr1::is_reference<T>::value == false, "Object type cannot be reference type");
public:
	typedef std::shared_ptr<PoolType> PoolTypePtr;

	ObjectPool(const Factory& factory, const std::function<void(T*)>& destructor) :	factory_(factory)
	{
		pPool_ = PoolTypePtr(new PoolType(), [=](PoolType* pPool)
			{
				T* pItem;
				while(pPool->try_pop(pItem))
					destructor(pItem);
				delete pPool;
			});
	}

	ObjectPool(Factory&& factory, std::function<void(T*)>&& destructor)	: factory_(std::move(factory))
	{
		pPool_ = PoolTypePtr(new PoolType(), [=](PoolType* pPool)
			{
				T* pItem;
				while(pPool->try_pop(pItem))
					destructor(pItem);
				delete pPool;
			});
	}

	std::shared_ptr<T> Create()
	{
		T* pItem = pPool_->try_pop(pItem) ? pItem : factory_();
		return std::shared_ptr<T>(pItem, [=](T* item){ pPool_->push(item); });
	}

	template<typename P0>
	std::shared_ptr<T> Create(P0&& p0)
	{
		T* pItem = pPool_->try_pop(pItem) ? pItem : factory_(std::forward<P0>(p0));
		return std::shared_ptr<T>(pItem, [=](T* item){ pPool_->push(item); });
	}

	template<typename P0, typename P1>
	std::shared_ptr<T> Create(P0&& p0, P1&& p1)
	{
		T* pItem = pPool_->try_pop(pItem) ? pItem : factory_(std::forward<P0>(p0), std::forward<P1>(p1));
		return std::shared_ptr<T>(pItem, [=](T* item){ pPool_->push(item); });
	}	

	template<typename P0, typename P1, typename P2>
	std::shared_ptr<T> Create(P0&& p0, P1&& p1, P2&& p2)
	{
		T* pItem = pPool_->try_pop(pItem) ? pItem : factory_(std::forward<P0>(p0), std::forward<P1>(p1), std::forward<P2>(p2));
		return std::shared_ptr<T>(pItem, [=](T* item){ pPool_->push(item); });
	}

	template<typename P0, typename P1, typename P2, typename P3>
	std::shared_ptr<T> Create(P0&& p0, P1&& p1, P2&& p2, P3&& p3)
	{
		T* pItem = pPool_->try_pop(pItem) ? pItem : factory_(std::forward<P0>(p0), std::forward<P1>(p1), std::forward<P2>(p2), std::forward<P3>(p3));
		return std::shared_ptr<T>(pItem, [=](T* item){ pPool_->push(item); });
	}

private:
	PoolTypePtr pPool_;
	const Factory factory_;
};

}
}

#endif