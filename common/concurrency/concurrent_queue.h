#pragma once

namespace caspar {

template<typename T>
class concurrent_bounded_queue_r
{
	typedef tbb::concurrent_bounded_queue<std::shared_ptr<T>> queue_t;
public:
	void push(T&& source) 
	{
        queue_.push(std::make_shared<T>(std::forward<T>(source)));
    }

    void pop(T& destination)
	{
		std::shared_ptr<T> container;
        queue_.pop(container);
		destination = std::move(*container);
    }

    bool try_push(T&& source) 
	{
		auto container = std::make_shared<T>(std::forward<T>(source));
		bool pushed = queue_.try_push(container);
		if(!pushed)
			source = std::move(*container);
        return pushed;
    }

    bool try_pop(T& destination) 
	{
		std::shared_ptr<T> container;
        bool popped = queue_.try_pop(container);
		if(popped)
			destination = std::move(*container);
		return popped;
    }

    typename queue_t::size_type size() const {return queue_.size();}

    bool empty() const {return queue_.empty();}

    typename queue_t::size_type capacity() const 
	{
        return queue_.capacity();
    }

    void set_capacity(typename queue_t::size_type new_capacity) 
	{
        queue_.set_capacity(new_capacity);
    }

	void clear()
	{
		queue_.clear();
	}

    typename queue_t::allocator_type get_allocator() const { return queue_.get_allocator(); }

private:
	queue_t queue_;
};

}