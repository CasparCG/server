/*
* Copyright (c) 2011 Sveriges Television AB <info@casparcg.com>
*
* This file is part of CasparCG (www.casparcg.com).
*
* CasparCG is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* CasparCG is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with CasparCG. If not, see <http://www.gnu.org/licenses/>.
*
* Author: Helge Norberg, helge.norberg@svt.se
*/

#pragma once

#include <functional>
#include <memory>
#include <vector>
#include <string>
#include <map>
#include <algorithm>
#include <type_traits>
#include <stdexcept>

#include <boost/lexical_cast.hpp>
#include <boost/utility/value_init.hpp>

#include <common/tweener.h>
#include <common/except.h>

namespace caspar { namespace core {

namespace detail {

struct impl_base : std::enable_shared_from_this<impl_base>
{
	std::vector<std::shared_ptr<impl_base>> dependencies_;
	mutable std::vector<std::pair<
			std::weak_ptr<void>,
			std::function<void ()>>> on_change_;

	virtual ~impl_base()
	{
	}

	virtual void evaluate() const = 0;

	void depend_on(const std::shared_ptr<impl_base>& dependency)
	{
		auto self = shared_from_this();

		if (dependency->depends_on(self))
			CASPAR_THROW_EXCEPTION(invalid_argument() << msg_info("Can't have circular dependencies between bindings"));

		dependency->on_change(self, [=] { evaluate(); });
		dependencies_.push_back(dependency);
	}

	bool depends_on(const std::shared_ptr<impl_base>& other) const
	{
		for (auto& dependency : dependencies_)
		{
			if (dependency == other)
				return true;

			if (dependency->depends_on(other))
				return true;
		}

		return false;
	}

	void on_change(
			const std::weak_ptr<void>& dependant,
			const std::function<void ()>& listener) const
	{
		on_change_.push_back(std::make_pair(dependant, listener));
	}
};

}

template <typename T>
class binding
{
private:

	struct impl : public detail::impl_base
	{
		mutable T			value_;
		std::function<T ()> expression_;
		mutable bool		evaluated_		= false;

		impl()
			: value_(boost::value_initialized<T>())
		{
		}

		impl(T value)
			: value_(value)
		{
		}

		template<typename Expr>
		impl(const Expr& expression)
			: value_(boost::value_initialized<T>())
			, expression_(expression)
		{
		}

		T get() const
		{
			if (!evaluated_)
				evaluate();

			return value_;
		}

		bool bound() const
		{
			return static_cast<bool>(expression_);
		}

		void set(T value)
		{
			if (bound())
				CASPAR_THROW_EXCEPTION(caspar_exception() << msg_info("Bound value cannot be set"));

			if (value == value_)
				return;

			value_ = value;

			on_change();
		}

		void evaluate() const override
		{
			if (bound())
			{
				auto new_value = expression_();

				if (new_value != value_)
				{
					value_ = new_value;
					on_change();
				}
			}

			evaluated_ = true;
		}

		using impl_base::on_change;
		void on_change() const
		{
			auto copy = on_change_;

			for (int i = static_cast<int>(copy.size()) - 1; i >= 0; --i)
			{
				auto strong = copy[i].first.lock();

				if (strong)
					copy[i].second();
				else
					on_change_.erase(on_change_.begin() + i);
			}
		}

		void bind(const std::shared_ptr<impl>& other)
		{
			unbind();
			depend_on(other);
			expression_ = [other]{ return other->get(); };
			evaluate();
		}

		void unbind()
		{
			if (bound())
			{
				expression_ = std::function<T ()>();
				dependencies_.clear();
			}
		}
	};

	template<typename> friend class binding;

	std::shared_ptr<impl> impl_;
public:
	binding()
		: impl_(new impl)
	{
	}

	explicit binding(T value)
		: impl_(new impl(value))
	{
	}

	// Expr -> T ()
	template<typename Expr>
	explicit binding(const Expr& expression)
		: impl_(new impl(expression))
	{
		//impl_->evaluate();
	}

	// Expr -> T ()
	template<typename Expr, typename T2>
	binding(const Expr& expression, const binding<T2>& dep)
		: impl_(new impl(expression))
	{
		depend_on(dep);
		//impl_->evaluate();
	}

	// Expr -> T ()
	template<typename Expr, typename T2, typename T3>
	binding(
			const Expr& expression,
			const binding<T2>& dep1,
			const binding<T3>& dep2)
		: impl_(new impl(expression))
	{
		depend_on(dep1);
		depend_on(dep2);
		//impl_->evaluate();
	}

	void* identity() const
	{
		return impl_.get();
	}

	T get() const
	{
		return impl_->get();
	}

	void set(T value)
	{
		impl_->set(value);
	}

	void bind(const binding<T>& other)
	{
		impl_->bind(other.impl_);
	}

	bool bound() const
	{
		return impl_->bound();
	}

	template<typename T2>
	void depend_on(const binding<T2>& other)
	{
		impl_->depend_on(other.impl_);
	}

	binding<T> operator+(T other) const
	{
		return transformed([other](T self) { return self + other; });
	}

	binding<T> operator+(const binding<T>& other) const
	{
		return composed(other, [](T self, T o) { return self + o; });
	}

	binding<T>& operator++()
	{
		T new_value = get();
		++new_value;

		set(new_value);

		return *this;
	}

	binding<T> operator++(int)
	{
		binding<T> pre_val(get());
		++*this;

		return pre_val;
	}

	binding<T> operator-() const
	{
		return transformed([](T self) { return -self; });
	}

	binding<T> operator-(const binding<T>& other) const
	{
		return *this + -other;
	}

	binding<T> operator-(T other) const
	{
		return *this + -other;
	}

	binding<T>& operator--()
	{
		T new_value = get();
		--new_value;

		set(new_value);

		return *this;
	}

	binding<T> operator--(int)
	{
		binding<T> pre_val(get());
		--*this;

		return pre_val;
	}

	binding<T> operator*(T other) const
	{
		return transformed([other](T self) { return self * other; });
	}

	binding<T> operator*(const binding<T>& other) const
	{
		return composed(other, [](T self, T o) { return self * o; });
	}

	binding<T> operator/(T other) const
	{
		return transformed([other](T self) { return self / other; });
	}

	binding<T> operator/(const binding<T>& other) const
	{
		return composed(other, [](T self, T o) { return self / o; });
	}

	binding<T> operator%(T other) const
	{
		return transformed([other](T self) { return self % other; });
	}

	binding<T> operator%(const binding<T>& other) const
	{
		return composed(other, [](T self, T o) { return self % o; });
	}

	binding<bool> operator==(T other) const
	{
		return transformed([other](T self) { return self == other; });
	}

	binding<bool> operator==(const binding<T>& other) const
	{
		return composed(other, [](T self, T o) { return self == o; });
	}

	binding<bool> operator!=(T other) const
	{
		return transformed([other](T self) { return self != other; });
	}

	binding<bool> operator!=(const binding<T>& other) const
	{
		return composed(other, [](T self, T o) { return self != o; });
	}

	binding<bool> operator<(T other) const
	{
		return transformed([other](T self) { return self < other; });
	}

	binding<bool> operator<(const binding<T>& other) const
	{
		return composed(other, [](T self, T o) { return self < o; });
	}

	binding<bool> operator>(T other) const
	{
		return transformed([other](T self) { return self > other; });
	}

	binding<bool> operator>(const binding<T>& other) const
	{
		return composed(other, [](T self, T o) { return self > o; });
	}

	binding<bool> operator<=(T other) const
	{
		return transformed([other](T self) { return self <= other; });
	}

	binding<bool> operator<=(const binding<T>& other) const
	{
		return composed(other, [](T self, T o) { return self <= o; });
	}

	binding<bool> operator>=(T other) const
	{
		return transformed([other](T self) { return self >= other; });
	}

	binding<bool> operator>=(const binding<T>& other) const
	{
		return composed(other, [](T self, T o) { return self >= o; });
	}

	template<typename T2>
	typename std::enable_if<
			std::is_same<T, T2>::value,
			binding<T2>
		>::type as() const
	{
		return *this;
	}

	template<typename T2>
	typename std::enable_if<
			(std::is_arithmetic<T>::value || std::is_same<bool, T>::value) && (std::is_arithmetic<T2>::value || std::is_same<bool, T2>::value) && !std::is_same<T, T2>::value,
			binding<T2>
		>::type as() const
	{
		return transformed([](T val) { return static_cast<T2>(val); });
	}

	template<typename T2>
	typename std::enable_if<
			(std::is_same<std::wstring, T>::value || std::is_same<std::wstring, T2>::value) && !std::is_same<T, T2>::value,
			binding<T2>
		>::type as() const
	{
		return transformed([](T val) { return boost::lexical_cast<T2>(val); });
	}

	// Func -> R (T self_val)
	// Returns binding<R>
	template<typename Func>
	auto transformed(const Func& func) const -> binding<decltype(func(impl_->value_))>
	{
		typedef decltype(func(impl_->value_)) R;
		auto self = impl_;

		return binding<R>(
				[self, func] { return func(self->get()); },
				*this);
	}

	// Func -> R (T self_val, T2 other_val)
	// Returns binding<R>
	template<typename Func, typename T2>
	auto composed(const binding<T2>& other, const Func& func) const -> binding<decltype(func(impl_->value_, other.impl_->value_))>
	{
		typedef decltype(func(impl_->value_, other.impl_->value_)) R;
		auto self = impl_;
		auto o = other.impl_;

		return binding<R>(
				[self, o, func] { return func(self->get(), o->get()); },
				*this,
				other);
	}

	template<typename TweenerFunc, typename FrameCounter>
	binding<T> animated(
			const binding<FrameCounter>& frame_counter,
			const binding<FrameCounter>& duration,
			const binding<TweenerFunc>& tweener_func) const
	{
		auto self					= impl_;
		auto f						= frame_counter.impl_;
		auto d						= duration.impl_;
		auto tw						= tweener_func.impl_;
		FrameCounter start_frame	= frame_counter.get();
		FrameCounter current_frame	= start_frame;
		T current_source			= get();
		T current_destination		= current_source;
		T current_result			= current_source;

		auto result = binding<T>(
			[=] () mutable
			{
				auto frame_diff		= f->get() - current_frame;
				bool new_frame		= f->get() != current_frame;

				if (!new_frame)
					return current_result;

				bool new_tween		= current_destination != self->get();
				auto time			= current_frame - start_frame + (new_tween ? frame_diff : 0) + 1;
				auto dur			= d->get();
				current_frame		= f->get();

				current_source		= new_tween ? tw->get()(time, current_source, current_destination - current_source, dur) : current_source;
				current_destination	= self->get();

				if (new_tween)
					start_frame		= current_frame;

				time				= current_frame - start_frame;

				if (time < dur)
					current_result = tw->get()(time, current_source, current_destination - current_source, dur);
				else
				{
					current_result = current_destination;
					current_source = current_destination;
				}

				return current_result;
			});

		result.depend_on(*this);
		result.depend_on(frame_counter);
		result.depend_on(tweener_func);

		return result;
	}

	void unbind()
	{
		impl_->unbind();
	}

	void on_change(
			const std::weak_ptr<void>& dependant,
			const std::function<void ()>& listener) const
	{
		impl_->on_change(dependant, listener);
	}

	std::shared_ptr<void> on_change(
			const std::function<void ()>& listener) const
	{
		std::shared_ptr<void> subscription(new char);

		on_change(subscription, listener);

		return subscription;
	}
private:
	binding(const std::shared_ptr<impl>& self)
		: impl_(self)
	{
	}
};

static binding<bool> operator||(const binding<bool>& lhs, const binding<bool>& rhs)
{
	return lhs.composed(rhs, [](bool lhs, bool rhs) { return lhs || rhs; });
}

static binding<bool> operator||(const binding<bool>& lhs, bool rhs)
{
	return lhs.transformed([rhs](bool lhs) { return lhs || rhs; });
}

static binding<bool> operator&&(const binding<bool>& lhs, const binding<bool>& rhs)
{
	return lhs.composed(rhs, [](bool lhs, bool rhs) { return lhs && rhs; });
}

static binding<bool> operator&&(const binding<bool>& lhs, bool rhs)
{
	return lhs.transformed([rhs](bool lhs) { return lhs && rhs; });
}

static binding<bool> operator!(const binding<bool>& self)
{
	return self.transformed([](bool self) { return !self; });
}

template<typename T>
class ternary_builder
{
	binding<bool> condition_;
	binding<T> true_result_;
public:
	ternary_builder(
			const binding<bool>& condition, const binding<T>& true_result)
		: condition_(condition)
		, true_result_(true_result)
	{
	}

	binding<T> otherwise(const binding<T>& false_result)
	{
		auto condition = condition_;
		auto true_result = true_result_;

		binding<T> result([condition, true_result, false_result]()
		{
			return condition.get() ? true_result.get() : false_result.get();
		});

		result.depend_on(condition);
		result.depend_on(true_result);
		result.depend_on(false_result);

		return result;
	}

	binding<T> otherwise(T false_result)
	{
		return otherwise(binding<T>(false_result));
	}
};

class when
{
	binding<bool> condition_;
public:
	when(const binding<bool>& condition)
		: condition_(condition)
	{
	}

	template<typename T>
	ternary_builder<T> then(const binding<T>& true_result)
	{
		return ternary_builder<T>(condition_, true_result);
	}

	template<typename T>
	ternary_builder<T> then(T true_result)
	{
		return then(binding<T>(true_result));
	}
};

/*template<typename T, typename T2>
binding<T> add_tween(
		const binding<T>& to_tween,
		const binding<T2>& counter,
		T destination_value,
		T2 duration,
		const std::wstring& easing)
{
	tweener tween(easing);

	double start_val = to_tween.as<double>().get();
	double destination_val = static_cast<double>(destination_value);
	double start_time = counter.as<double>().get();
	double dur = static_cast<double>(duration);

	return when(counter < duration)
		.then(counter.as<double>().transformed([=](double t)
		{
			return tween(t - start_time, start_val, destination_val, dur);
		}).as<T>())
		.otherwise(destination_value);
}*/

template<typename T, typename T2>
binding<T> delay(
		const binding<T>& to_delay,
		const binding<T>& after_delay,
		const binding<T2>& counter,
		T2 duration)
{
	return when(counter < duration)
			.then(to_delay)
			.otherwise(after_delay);
}

}}
