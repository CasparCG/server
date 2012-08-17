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
* Author: Robert Nagy, ronag89@gmail.com
*/

#pragma once

#include <memory>
#include <stdexcept>
#include <type_traits>

namespace caspar
{

template<typename T>
class safe_ptr
{   
    template <typename> friend class safe_ptr;
public:
    typedef T  element_type;

    safe_ptr(); // will construct new T object using make_safe<T>()

    safe_ptr(const safe_ptr& other) 
        : p_(other.p_)
    {
    }

    template<typename U>
    safe_ptr(const safe_ptr<U>& other, typename std::enable_if<std::is_convertible<U*, T*>::value, void*>::type = 0) 
        : p_(other.p_)
    {
    }

    safe_ptr(safe_ptr&& other) 
        : p_(other.p_)
    {
    }

    template<typename U>
    safe_ptr(safe_ptr<U>&& other, typename std::enable_if<std::is_convertible<U*, T*>::value, void*>::type = 0) 
        : p_(other.p_)
    {
    }
	
    template<typename U>    
    explicit safe_ptr(const std::shared_ptr<U>& p, typename std::enable_if<std::is_convertible<U*, T*>::value, void*>::type = 0) 
        : p_(p)
    {
        if(!p)
            throw std::invalid_argument("p");
    }

    template<typename U>    
    explicit safe_ptr(std::shared_ptr<U>&& p, typename std::enable_if<std::is_convertible<U*, T*>::value, void*>::type = 0) 
        : p_(std::move(p))
    {
        if(!p_)
            throw std::invalid_argument("p");
    }

    template<typename U>    
    explicit safe_ptr(U* p, typename std::enable_if<std::is_convertible<U*, T*>::value, void*>::type = 0) 
        : p_(p)
    {
        if(!p)
            throw std::invalid_argument("p");
    }

    template<typename U, typename D>    
    explicit safe_ptr(U* p, D d, typename std::enable_if<std::is_convertible<U*, T*>::value, void*>::type = 0) 
        : p_(p, d)
    {
        if(!p)
            throw std::invalid_argument("p");
    }
    
    template<typename U>
    typename std::enable_if<std::is_convertible<U*, T*>::value, safe_ptr&>::type
    operator=(const safe_ptr<U>& other)
    {
        safe_ptr(other).swap(*this);
        return *this;
    }

    template<typename U>
    typename std::enable_if<std::is_convertible<U*, T*>::value, safe_ptr&>::type
    operator=(safe_ptr<U>&& other)
    {
        safe_ptr<T>(std::move(other)).swap(*this);
        return *this;
    }
	
    T& operator*() const 
    { 
        return *p_.get();
    }

    T* operator->() const 
    { 
        return p_.get();
    }

    T* get() const 
    { 
        return p_.get();
    }

    bool unique() const 
    { 
        return p_.unique();
    }

    long use_count() const 
    {
        return p_.use_count();
    }

    void swap(safe_ptr& other) 
    { 
        p_.swap(other.p_); 
    } 

    operator std::shared_ptr<T>() const 
    { 
        return p_;
    }

    operator std::weak_ptr<T>() const 
    { 
        return std::weak_ptr<T>(p_);
    }
    
    template<class U>
    bool owner_before(const safe_ptr& ptr)
    { 
        return p_.owner_before(ptr.p_); 
    }

    template<class U>
    bool owner_before(const std::shared_ptr<U>& ptr)
    { 
        return p_.owner_before(ptr); 
    }

    template<class D, class U> 
    D* get_deleter(safe_ptr<U> const& ptr) 
    { 
        return p_.get_deleter(); 
    }

private:    
    std::shared_ptr<T> p_;
};

template<class T, class U>
bool operator==(const safe_ptr<T>& a, const safe_ptr<U>& b)
{
    return a.get() == b.get();
}

template<class T, class U>
bool operator==(const std::shared_ptr<T>& a, const safe_ptr<U>& b)
{
    return a.get() == b.get();
}

template<class T, class U>
bool operator==(const safe_ptr<T>& a, const std::shared_ptr<U>& b)
{
    return a.get() == b.get();
}

template<class T, class U>
bool operator!=(const safe_ptr<T>& a, const safe_ptr<U>& b)
{
    return a.get() != b.get();
}

template<class T, class U>
bool operator!=(const std::shared_ptr<T>& a, const safe_ptr<U>& b)
{
    return a.get() != b.get();
}

template<class T, class U>
bool operator!=(const safe_ptr<T>& a, const std::shared_ptr<U>& b)
{
    return a.get() != b.get();
}

template<class T, class U>
bool operator<(const safe_ptr<T>& a, const safe_ptr<U>& b)
{
    return a.get() < b.get();
}

template<class T, class U>
bool operator<(const std::shared_ptr<T>& a, const safe_ptr<U>& b)
{
    return a.get() < b.get();
}

template<class T, class U>
bool operator<(const safe_ptr<T>& a, const std::shared_ptr<U>& b)
{
    return a.get() < b.get();
}

template<class T, class U>
bool operator>(const safe_ptr<T>& a, const safe_ptr<U>& b)
{
    return a.get() > b.get();
}

template<class T, class U>
bool operator>(const std::shared_ptr<T>& a, const safe_ptr<U>& b)
{
    return a.get() > b.get();
}

template<class T, class U>
bool operator>(const safe_ptr<T>& a, const std::shared_ptr<U>& b)
{
    return a.get() > b.get();
}

template<class T, class U>
bool operator>=(const safe_ptr<T>& a, const safe_ptr<U>& b)
{
    return a.get() >= b.get();
}

template<class T, class U>
bool operator>=(const std::shared_ptr<T>& a, const safe_ptr<U>& b)
{
    return a.get() >= b.get();
}

template<class T, class U>
bool operator>=(const safe_ptr<T>& a, const std::shared_ptr<U>& b)
{
    return a.get() >= b.get();
}

template<class T, class U>
bool operator<=(const safe_ptr<T>& a, const safe_ptr<U>& b)
{
    return a.get() <= b.get();
}

template<class T, class U>
bool operator<=(const std::shared_ptr<T>& a, const safe_ptr<U>& b)
{
    return a.get() <= b.get();
}

template<class T, class U>
bool operator<=(const safe_ptr<T>& a, const std::shared_ptr<U>& b)
{
    return a.get() <= b.get();
}

template<class E, class T, class U>
std::basic_ostream<E, T>& operator<<(std::basic_ostream<E, T>& out, const safe_ptr<U>& p)
{
    return out << p.get();
}

template<class T> 
void swap(safe_ptr<T>& a, safe_ptr<T>& b)
{
    a.swap(b);
}

template<class T> 
T* get_pointer(safe_ptr<T> const& p)
{
    return p.get();
}

template <class T, class U>
safe_ptr<T> static_pointer_cast(const safe_ptr<U>& p)
{
    return safe_ptr<T>(std::static_pointer_cast<T>(std::shared_ptr<U>(p)));
}

template <class T, class U>
safe_ptr<T> const_pointer_cast(const safe_ptr<U>& p)
{
    return safe_ptr<T>(std::const_pointer_cast<T>(std::shared_ptr<U>(p)));
}

template <class T, class U>
safe_ptr<T> dynamic_pointer_cast(const safe_ptr<U>& p)
{
    auto temp = std::dynamic_pointer_cast<T>(std::shared_ptr<U>(p));
    if(!temp)
        throw std::bad_cast();
    return safe_ptr<T>(std::move(temp));
}

//
// enable_safe_this 
//
// A safe_ptr version of enable_shared_from_this.
// So that an object may get safe_ptr objects to itself.
//

template<class T>
class enable_safe_from_this : public std::enable_shared_from_this<T>
{
public:
    safe_ptr<T> safe_from_this() 
    {
        return safe_ptr<T>(this->shared_from_this());
    }

    safe_ptr<T const> safe_from_this() const 
    {
        return safe_ptr<T const>(this->shared_from_this());
    }
protected:
    enable_safe_from_this()
    {
    }
    
    enable_safe_from_this(const enable_safe_from_this&)
    {
    }
    
    enable_safe_from_this& operator=(const enable_safe_from_this&)
    {        
        return *this;
    }
    
    ~enable_safe_from_this ()
    {
    }
};

//
// make_safe
//
// safe_ptr equivalents to make_shared
//

template<typename T>
safe_ptr<T> make_safe_ptr(const std::shared_ptr<T>& ptr)
{
	return safe_ptr<T>(ptr);
}

template<typename T>
safe_ptr<T> make_safe_ptr(std::shared_ptr<T>&& ptr)
{
	return safe_ptr<T>(ptr);
}

template<typename T>
safe_ptr<T> make_safe()
{
    return safe_ptr<T>(std::make_shared<T>());
}

template<typename T, typename P0>
safe_ptr<T> make_safe(P0&& p0)
{
    return safe_ptr<T>(std::make_shared<T>(std::forward<P0>(p0)));
}

template<typename T, typename P0, typename P1>
safe_ptr<T> make_safe(P0&& p0, P1&& p1)
{
    return safe_ptr<T>(std::make_shared<T>(std::forward<P0>(p0), std::forward<P1>(p1)));
}

template<typename T, typename P0, typename P1, typename P2>
safe_ptr<T> make_safe(P0&& p0, P1&& p1, P2&& p2)
{
    return safe_ptr<T>(std::make_shared<T>(std::forward<P0>(p0), std::forward<P1>(p1), std::forward<P2>(p2)));
}

template<typename T, typename P0, typename P1, typename P2, typename P3>
safe_ptr<T> make_safe(P0&& p0, P1&& p1, P2&& p2, P3&& p3)
{
    return safe_ptr<T>(std::make_shared<T>(std::forward<P0>(p0), std::forward<P1>(p1), std::forward<P2>(p2), std::forward<P3>(p3)));
}

template<typename T, typename P0, typename P1, typename P2, typename P3, typename P4>
safe_ptr<T> make_safe(P0&& p0, P1&& p1, P2&& p2, P3&& p3, P4&& p4)
{
    return safe_ptr<T>(std::make_shared<T>(std::forward<P0>(p0), std::forward<P1>(p1), std::forward<P2>(p2), std::forward<P3>(p3), std::forward<P4>(p4)));
}

template<typename T, typename P0, typename P1, typename P2, typename P3, typename P4, typename P5>
safe_ptr<T> make_safe(P0&& p0, P1&& p1, P2&& p2, P3&& p3, P4&& p4, P5&& p5)
{
    return safe_ptr<T>(std::make_shared<T>(std::forward<P0>(p0), std::forward<P1>(p1), std::forward<P2>(p2), std::forward<P3>(p3), std::forward<P4>(p4), std::forward<P5>(p5)));
}

template<typename T, typename P0, typename P1, typename P2, typename P3, typename P4, typename P5, typename P6>
safe_ptr<T> make_safe(P0&& p0, P1&& p1, P2&& p2, P3&& p3, P4&& p4, P5&& p5, P6&& p6)
{
	return safe_ptr<T>(std::make_shared<T>(std::forward<P0>(p0), std::forward<P1>(p1), std::forward<P2>(p2), std::forward<P3>(p3), std::forward<P4>(p4), std::forward<P5>(p5), std::forward<P6>(p6)));
}

template<typename T>
safe_ptr<T>::safe_ptr() 
    : p_(make_safe<T>())
{
} 

} // namespace
