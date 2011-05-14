#pragma once

namespace caspar {

template<typename T>
typename T::value_type pop_front(T& container)
{
	auto item = std::move(container.front());
	container.pop_front();
	return std::move(item);
}

template<typename T>
std::vector<T> split(const T& container, size_t size)
{
	std::vector<T> result;
	const auto last = container.end() - container.size() % size;	
	for(auto it = container.begin(); it != last; it += size)
		result.push_back(T(it, it + size));
	result.push_back(T(last, container.end()));

	return std::move(result);
}

}