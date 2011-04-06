#pragma once

#include <string>

#include <tbb/spin_mutex.h>

namespace caspar {

//class object
//{
//	mutable tbb::spin_mutex mutex_;
//	const object* parent_;
//public:
//	object(const object* parent = nullptr){set_parent(parent);}
//
//	std::wstring print() const
//	{
//		tbb::spin_mutex::scoped_lock lock(mutex_);
//		return (parent_ ? parent_->print() + L"/" : L"") + do_print();
//	}
//		
//	const object* get_parent() const
//	{
//		tbb::spin_mutex::scoped_lock lock(mutex_);
//		return parent_;
//	}
//
//	void set_parent(const object* parent)
//	{
//		tbb::spin_mutex::scoped_lock lock(mutex_);
//		parent_ = parent != this ? parent : nullptr;
//	}
//
//private:
//
//	virtual std::wstring do_print() const = 0;
//};

class object
{
	mutable tbb::spin_mutex mutex_;
	const object* parent_;
public:
	object(const object* parent = nullptr){set_parent(parent);}

	virtual std::wstring print() const
	{
		tbb::spin_mutex::scoped_lock lock(mutex_);
		return (parent_ ? parent_->print() + L"/" : L"");
	}
		
	const object* get_parent() const
	{
		tbb::spin_mutex::scoped_lock lock(mutex_);
		return parent_;
	}

	void set_parent(const object* parent)
	{
		tbb::spin_mutex::scoped_lock lock(mutex_);
		parent_ = parent != this ? parent : nullptr;
	}
};

}