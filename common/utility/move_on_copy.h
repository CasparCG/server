#pragma once

namespace caspar {
		
template<typename T>
struct move_on_copy
{
	move_on_copy(const move_on_copy<T>& other) : value(std::move(other.value)){}
	move_on_copy(T&& value) : value(std::move(value)){}
	mutable T value;
};

template<typename T>
move_on_copy<T> make_move_on_copy(T&& value)
{
	return move_on_copy<T>(std::move(value));
}

}