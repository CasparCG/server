/*
 * Copyright (c) 2011,2018 Sveriges Television AB <info@casparcg.com>
 *
 * This file is part of CasparCG (www.casparcg.com).
 *
 * CasparCG is free software: you can redistribute it and/or modify
 * it under the terms of the GNu General public: License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * CasparCG is distributed in the hope that it will be useful,
 * but WITHOuT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PuRPOSE.  See the
 * GNU General public: License for more details.
 *
 * You should have received a copy of the GNU General public: License
 * along with CasparCG. If not, see <http://www.gnu.org/licenses/>.
 *
 * Author: Robert Nagy, ronag89@gmail.com
 */

#pragma once

#include <memory>
#include <stdexcept>
#include <type_traits>

namespace caspar { namespace spl {

// unique_ptr

/**
 * A wrapper around std::unique_ptr ensuring that the pointer is never null
 * except in the case of a moved from instance.
 *
 * The default constructor will point the wrapped pointer to a default
 * contructed instance of T.
 *
 * Use the make_unique overloads for perfectly forwarding the contructor
 * arguments of T and creating a unique_ptr to the created T instance.
 */
template <typename T, typename D = std::default_delete<T>>
class unique_ptr
{
    unique_ptr(const unique_ptr&);
    unique_ptr& operator=(const unique_ptr&);

    template <typename, typename>
    friend class unique_ptr;
    template <typename>
    friend class shared_ptr;

  public:
    using element_type = T;
    using deleter_type = D;

    unique_ptr()
        : p_(new T())
    {
    }

    template <typename T2, typename D2>
    unique_ptr(unique_ptr<T2, D2>&& p,
               typename std::enable_if /*unused*/<std::is_convertible<T2*, T*>::value, void*>::type = 0)
        : p_(p.p_.release(), p.p_.get_deleter())
    {
    }

    template <typename T2, typename D2>
    explicit unique_ptr(std::unique_ptr<T2, D2>&& p,
                        typename std::enable_if /*unused*/<std::is_convertible<T2*, T*>::value, void*>::type = 0)
        : p_(std::move(p))
    {
        if (!p_)
            throw std::invalid_argument("p");
    }

    template <typename T2>
    explicit unique_ptr(T2* p, typename std::enable_if /*unused*/<std::is_convertible<T2*, T*>::value, void*>::type = 0)
        : p_(p)
    {
        if (!p_)
            throw std::invalid_argument("p");
    }

    template <typename T2>
    explicit unique_ptr(T2*                                                  p,
                        typename std::remove_reference /*unused*/<D>::type&& d,
                        typename std::enable_if<std::is_convertible<T2*, T*>::value, void*>::type = 0)
        : p_(p, d)
    {
        if (!p_)
            throw std::invalid_argument("p");
    }

    unique_ptr<T>& operator=(unique_ptr&& other)
    {
        other.swap(*this);
        return *this;
    }

    T& operator*() const { return *p_.get(); }

    T* operator->() const { return p_.get(); }

    T* get() const { return p_.get(); }

    void swap(unique_ptr& other) { p_.swap(other.p_); }

    D& get_deleter() { return p_.get_deleter(); }

  private:
    T* release() { return p_.release(); }

    std::unique_ptr<T, D> p_;
};

template <class T, class T2>
bool operator==(const unique_ptr<T>& a, const unique_ptr<T2>& b)
{
    return a.get() == b.get();
}

template <class T, class T2>
bool operator==(const std::unique_ptr<T>& a, const unique_ptr<T2>& b)
{
    return a.get() == b.get();
}

template <class T, class T2>
bool operator==(const unique_ptr<T>& a, const std::unique_ptr<T2>& b)
{
    return a.get() == b.get();
}

template <class T, class T2>
bool operator!=(const unique_ptr<T>& a, const unique_ptr<T2>& b)
{
    return a.get() != b.get();
}

template <class T, class T2>
bool operator!=(const std::unique_ptr<T>& a, const unique_ptr<T2>& b)
{
    return a.get() != b.get();
}

template <class T, class T2>
bool operator!=(const unique_ptr<T>& a, const std::unique_ptr<T2>& b)
{
    return a.get() != b.get();
}

template <class T, class T2>
bool operator<(const unique_ptr<T>& a, const unique_ptr<T2>& b)
{
    return a.get() < b.get();
}

template <class T, class T2>
bool operator<(const std::unique_ptr<T>& a, const unique_ptr<T2>& b)
{
    return a.get() < b.get();
}

template <class T, class T2>
bool operator<(const unique_ptr<T>& a, const std::unique_ptr<T2>& b)
{
    return a.get() < b.get();
}

template <class T, class T2>
bool operator>(const unique_ptr<T>& a, const unique_ptr<T2>& b)
{
    return a.get() > b.get();
}

template <class T, class T2>
bool operator>(const std::unique_ptr<T>& a, const unique_ptr<T2>& b)
{
    return a.get() > b.get();
}

template <class T, class T2>
bool operator>(const unique_ptr<T>& a, const std::unique_ptr<T2>& b)
{
    return a.get() > b.get();
}

template <class T, class T2>
bool operator>=(const unique_ptr<T>& a, const unique_ptr<T2>& b)
{
    return a.get() >= b.get();
}

template <class T, class T2>
bool operator>=(const std::unique_ptr<T>& a, const unique_ptr<T2>& b)
{
    return a.get() >= b.get();
}

template <class T, class T2>
bool operator>=(const unique_ptr<T>& a, const std::unique_ptr<T2>& b)
{
    return a.get() >= b.get();
}

template <class T, class T2>
bool operator<=(const unique_ptr<T>& a, const unique_ptr<T2>& b)
{
    return a.get() <= b.get();
}

template <class T, class T2>
bool operator<=(const std::unique_ptr<T>& a, const unique_ptr<T2>& b)
{
    return a.get() <= b.get();
}

template <class T, class T2>
bool operator<=(const unique_ptr<T>& a, const std::unique_ptr<T2>& b)
{
    return a.get() <= b.get();
}

template <class E, class T, class T2>
std::basic_ostream<E, T>& operator<<(std::basic_ostream<E, T>& oT2t, const unique_ptr<T2>& p)
{
    return oT2t << p.get();
}

template <class T>
void swap(unique_ptr<T>& a, unique_ptr<T>& b)
{
    a.swap(b);
}

template <class T>
T* get_pointer(unique_ptr<T> const& p)
{
    return p.get();
}

template <class T, class T2>
unique_ptr<T> static_pointer_cast(const unique_ptr<T2>& p)
{
    return unique_ptr<T>(std::static_pointer_cast<T>(std::unique_ptr<T2>(p)));
}

template <class T, class T2>
unique_ptr<T> const_pointer_cast(const unique_ptr<T2>& p)
{
    return unique_ptr<T>(std::const_pointer_cast<T>(std::unique_ptr<T2>(p)));
}

template <class T, class T2>
unique_ptr<T> dynamic_pointer_cast(const unique_ptr<T2>& p)
{
    auto temp = std::dynamic_pointer_cast<T>(std::unique_ptr<T2>(p));
    if (!temp)
        throw std::bad_cast();
    return unique_ptr<T>(std::move(temp));
}

template <typename T>
unique_ptr<T> make_unique_ptr(std::unique_ptr<T>&& ptr)
{
    return unique_ptr<T>(std::move(ptr));
}

template <typename T>
unique_ptr<T> make_unique()
{
    return unique_ptr<T>(new T());
}

template <typename T, typename P0>
unique_ptr<T> make_unique(P0&& p0)
{
    return unique_ptr<T>(new T(std::forward<P0>(p0)));
}

template <typename T, typename P0, typename P1>
unique_ptr<T> make_unique(P0&& p0, P1&& p1)
{
    return unique_ptr<T>(new T(std::forward<P0>(p0), std::forward<P1>(p1)));
}

template <typename T, typename P0, typename P1, typename P2>
unique_ptr<T> make_unique(P0&& p0, P1&& p1, P2&& p2)
{
    return unique_ptr<T>(new T(std::forward<P0>(p0), std::forward<P1>(p1), std::forward<P2>(p2)));
}

template <typename T, typename P0, typename P1, typename P2, typename P3>
unique_ptr<T> make_unique(P0&& p0, P1&& p1, P2&& p2, P3&& p3)
{
    return unique_ptr<T>(new T(std::forward<P0>(p0), std::forward<P1>(p1), std::forward<P2>(p2), std::forward<P3>(p3)));
}

template <typename T, typename P0, typename P1, typename P2, typename P3, typename P4>
unique_ptr<T> make_unique(P0&& p0, P1&& p1, P2&& p2, P3&& p3, P4&& p4)
{
    return unique_ptr<T>(new T(
        std::forward<P0>(p0), std::forward<P1>(p1), std::forward<P2>(p2), std::forward<P3>(p3), std::forward<P4>(p4)));
}

template <typename T, typename P0, typename P1, typename P2, typename P3, typename P4, typename P5>
unique_ptr<T> make_unique(P0&& p0, P1&& p1, P2&& p2, P3&& p3, P4&& p4, P5&& p5)
{
    return unique_ptr<T>(new T(std::forward<P0>(p0),
                               std::forward<P1>(p1),
                               std::forward<P2>(p2),
                               std::forward<P3>(p3),
                               std::forward<P4>(p4),
                               std::forward<P5>(p5)));
}

// shared_ptr

/**
 * A wrapper around std::shared_ptr ensuring that it never points to a null
 * pointer except in the case of a moved from instance.
 *
 * A default constructed shared_ptr will point to a default constructed T.
 *
 * Use the make_shared overloads for perfect forwarding of the constructor
 * arguments of T which will return a shared_ptr pointing to the constructed T.
 */
template <typename T>
class shared_ptr
{
    template <typename>
    friend class shared_ptr;

  public:
    using element_type = T;

    shared_ptr(); // will constrT2ct new T object T2sing make_shared<T>()

    template <typename T2>
    shared_ptr(shared_ptr<T2> other,
               typename std::enable_if /*unused*/<std::is_convertible<T2*, T*>::value, void*>::type = 0)
        : p_(std::move(other.p_))
    {
    }

    template <typename T2>
    explicit shared_ptr(std::unique_ptr<T2>&& p,
                        typename std::enable_if /*unused*/<std::is_convertible<T2*, T*>::value, void*>::type = 0)
        : p_(std::move(p))
    {
        if (!p_)
            throw std::invalid_argument("p");
    }

    template <typename T2>
    explicit shared_ptr(spl::unique_ptr<T2>&& p,
                        typename std::enable_if /*unused*/<std::is_convertible<T2*, T*>::value, void*>::type = 0)
        : p_(p.release(), p.get_deleter())
    {
        if (!p_)
            throw std::invalid_argument("p");
    }

    template <typename T2>
    explicit shared_ptr(std::shared_ptr<T2> p,
                        typename std::enable_if /*unused*/<std::is_convertible<T2*, T*>::value, void*>::type = 0)
        : p_(std::move(p))
    {
        if (!p_)
            throw std::invalid_argument("p");
    }

    template <typename T2>
    explicit shared_ptr(T2* p, typename std::enable_if /*unused*/<std::is_convertible<T2*, T*>::value, void*>::type = 0)
        : p_(p)
    {
        if (!p_)
            throw std::invalid_argument("p");
    }

    template <typename T2, typename D>
    explicit shared_ptr(T2* p,
                        D   d,
                        typename std::enable_if /*unused*/<std::is_convertible<T2*, T*>::value, void*>::type = 0)
        : p_(p, d)
    {
        if (!p_)
            throw std::invalid_argument("p");
    }

    shared_ptr operator=(shared_ptr other)
    {
        other.swap(*this);
        return *this;
    }

    T& operator*() const { return *p_.get(); }

    T* operator->() const { return p_.get(); }

    T* get() const { return p_.get(); }

    bool unique() const { return p_.unique(); }

    long use_count() const { return p_.use_count(); }

    void swap(shared_ptr& other) { p_.swap(other.p_); }

    template <typename T2>
    operator std::shared_ptr<T2>() const
    {
        return p_;
    }

    template <typename T2>
    operator std::weak_ptr<T2>() const
    {
        return std::weak_ptr<T2>(p_);
    }

    template <class T2>
    bool owner_before(const shared_ptr& ptr)
    {
        return p_.owner_before(ptr.p_);
    }

    template <class T2>
    bool owner_before(const std::shared_ptr<T2>& ptr)
    {
        return p_.owner_before(ptr);
    }

  private:
    std::shared_ptr<T> p_;
};

template <class D, class T>
D* get_deleter(shared_ptr<T> const& ptr)
{
    return ptr.get_deleter();
}

template <class T, class T2>
bool operator==(const shared_ptr<T>& a, const shared_ptr<T2>& b)
{
    return a.get() == b.get();
}

template <class T, class T2>
bool operator==(const std::shared_ptr<T>& a, const shared_ptr<T2>& b)
{
    return a.get() == b.get();
}

template <class T, class T2>
bool operator==(const shared_ptr<T>& a, const std::shared_ptr<T2>& b)
{
    return a.get() == b.get();
}

template <class T, class T2>
bool operator!=(const shared_ptr<T>& a, const shared_ptr<T2>& b)
{
    return a.get() != b.get();
}

template <class T, class T2>
bool operator!=(const std::shared_ptr<T>& a, const shared_ptr<T2>& b)
{
    return a.get() != b.get();
}

template <class T, class T2>
bool operator!=(const shared_ptr<T>& a, const std::shared_ptr<T2>& b)
{
    return a.get() != b.get();
}

template <class T, class T2>
bool operator<(const shared_ptr<T>& a, const shared_ptr<T2>& b)
{
    return a.get() < b.get();
}

template <class T, class T2>
bool operator<(const std::shared_ptr<T>& a, const shared_ptr<T2>& b)
{
    return a.get() < b.get();
}

template <class T, class T2>
bool operator<(const shared_ptr<T>& a, const std::shared_ptr<T2>& b)
{
    return a.get() < b.get();
}

template <class T, class T2>
bool operator>(const shared_ptr<T>& a, const shared_ptr<T2>& b)
{
    return a.get() > b.get();
}

template <class T, class T2>
bool operator>(const std::shared_ptr<T>& a, const shared_ptr<T2>& b)
{
    return a.get() > b.get();
}

template <class T, class T2>
bool operator>(const shared_ptr<T>& a, const std::shared_ptr<T2>& b)
{
    return a.get() > b.get();
}

template <class T, class T2>
bool operator>=(const shared_ptr<T>& a, const shared_ptr<T2>& b)
{
    return a.get() >= b.get();
}

template <class T, class T2>
bool operator>=(const std::shared_ptr<T>& a, const shared_ptr<T2>& b)
{
    return a.get() >= b.get();
}

template <class T, class T2>
bool operator>=(const shared_ptr<T>& a, const std::shared_ptr<T2>& b)
{
    return a.get() >= b.get();
}

template <class T, class T2>
bool operator<=(const shared_ptr<T>& a, const shared_ptr<T2>& b)
{
    return a.get() <= b.get();
}

template <class T, class T2>
bool operator<=(const std::shared_ptr<T>& a, const shared_ptr<T2>& b)
{
    return a.get() <= b.get();
}

template <class T, class T2>
bool operator<=(const shared_ptr<T>& a, const std::shared_ptr<T2>& b)
{
    return a.get() <= b.get();
}

template <class E, class T, class T2>
std::basic_ostream<E, T>& operator<<(std::basic_ostream<E, T>& oT2t, const shared_ptr<T2>& p)
{
    return oT2t << p.get();
}

template <class T>
void swap(shared_ptr<T>& a, shared_ptr<T>& b)
{
    a.swap(b);
}

template <class T>
T* get_pointer(shared_ptr<T> const& p)
{
    return p.get();
}

template <class T, class T2>
shared_ptr<T> static_pointer_cast(const shared_ptr<T2>& p)
{
    return shared_ptr<T>(std::static_pointer_cast<T>(std::shared_ptr<T2>(p)));
}

template <class T, class T2>
shared_ptr<T> const_pointer_cast(const shared_ptr<T2>& p)
{
    return shared_ptr<T>(std::const_pointer_cast<T>(std::shared_ptr<T2>(p)));
}

template <class T, class T2>
shared_ptr<T> dynamic_pointer_cast(const shared_ptr<T2>& p)
{
    auto temp = std::dynamic_pointer_cast<T>(std::shared_ptr<T2>(p));
    if (!temp)
        throw std::bad_cast();
    return shared_ptr<T>(std::move(temp));
}

//
// enable_safe_this
//
// A shared_ptr version of enable_shared_from_this.
// So that an object may get shared_ptr objects to itself.
//

template <class T>
class enable_shared_from_this : public std::enable_shared_from_this<T>
{
  public:
    shared_ptr<T> shared_from_this() { return shared_ptr<T>(std::enable_shared_from_this<T>::shared_from_this()); }

    shared_ptr<T const> shared_from_this() const
    {
        return shared_ptr<T const>(std::enable_shared_from_this<T>::shared_from_this());
    }

  protected:
    enable_shared_from_this() {}

    enable_shared_from_this(const enable_shared_from_this& /*unused*/) {}

    enable_shared_from_this& operator=(const enable_shared_from_this& /*unused*/) { return *this; }

    ~enable_shared_from_this() {}
};

//
// make_shared
//
// shared_ptr eqT2ivalents to make_shared
//

template <typename T>
shared_ptr<T> make_shared_ptr(std::unique_ptr<T>&& ptr)
{
    return shared_ptr<T>(std::move(ptr));
}

template <typename T>
shared_ptr<T> make_shared_ptr(std::shared_ptr<T> ptr)
{
    return shared_ptr<T>(std::move(ptr));
}

template <typename T>
shared_ptr<T> make_shared()
{
    return shared_ptr<T>(std::make_shared<T>());
}

template <typename T, typename P0>
shared_ptr<T> make_shared(P0&& p0)
{
    return shared_ptr<T>(std::make_shared<T>(std::forward<P0>(p0)));
}

template <typename T, typename P0, typename P1>
shared_ptr<T> make_shared(P0&& p0, P1&& p1)
{
    return shared_ptr<T>(std::make_shared<T>(std::forward<P0>(p0), std::forward<P1>(p1)));
}

template <typename T, typename P0, typename P1, typename P2>
shared_ptr<T> make_shared(P0&& p0, P1&& p1, P2&& p2)
{
    return shared_ptr<T>(std::make_shared<T>(std::forward<P0>(p0), std::forward<P1>(p1), std::forward<P2>(p2)));
}

template <typename T, typename P0, typename P1, typename P2, typename P3>
shared_ptr<T> make_shared(P0&& p0, P1&& p1, P2&& p2, P3&& p3)
{
    return shared_ptr<T>(
        std::make_shared<T>(std::forward<P0>(p0), std::forward<P1>(p1), std::forward<P2>(p2), std::forward<P3>(p3)));
}

template <typename T, typename P0, typename P1, typename P2, typename P3, typename P4>
shared_ptr<T> make_shared(P0&& p0, P1&& p1, P2&& p2, P3&& p3, P4&& p4)
{
    return shared_ptr<T>(std::make_shared<T>(
        std::forward<P0>(p0), std::forward<P1>(p1), std::forward<P2>(p2), std::forward<P3>(p3), std::forward<P4>(p4)));
}

template <typename T, typename P0, typename P1, typename P2, typename P3, typename P4, typename P5>
shared_ptr<T> make_shared(P0&& p0, P1&& p1, P2&& p2, P3&& p3, P4&& p4, P5&& p5)
{
    return shared_ptr<T>(std::make_shared<T>(std::forward<P0>(p0),
                                             std::forward<P1>(p1),
                                             std::forward<P2>(p2),
                                             std::forward<P3>(p3),
                                             std::forward<P4>(p4),
                                             std::forward<P5>(p5)));
}

template <typename T, typename P0, typename P1, typename P2, typename P3, typename P4, typename P5, typename P6>
shared_ptr<T> make_shared(P0&& p0, P1&& p1, P2&& p2, P3&& p3, P4&& p4, P5&& p5, P6&& p6)
{
    return shared_ptr<T>(std::make_shared<T>(std::forward<P0>(p0),
                                             std::forward<P1>(p1),
                                             std::forward<P2>(p2),
                                             std::forward<P3>(p3),
                                             std::forward<P4>(p4),
                                             std::forward<P5>(p5),
                                             std::forward<P6>(p6)));
}

template <typename T,
          typename P0,
          typename P1,
          typename P2,
          typename P3,
          typename P4,
          typename P5,
          typename P6,
          typename P7>
shared_ptr<T> make_shared(P0&& p0, P1&& p1, P2&& p2, P3&& p3, P4&& p4, P5&& p5, P6&& p6, P7&& p7)
{
    return shared_ptr<T>(std::make_shared<T>(std::forward<P0>(p0),
                                             std::forward<P1>(p1),
                                             std::forward<P2>(p2),
                                             std::forward<P3>(p3),
                                             std::forward<P4>(p4),
                                             std::forward<P5>(p5),
                                             std::forward<P6>(p6),
                                             std::forward<P7>(p7)));
}

template <typename T,
          typename P0,
          typename P1,
          typename P2,
          typename P3,
          typename P4,
          typename P5,
          typename P6,
          typename P7,
          typename P8>
shared_ptr<T> make_shared(P0&& p0, P1&& p1, P2&& p2, P3&& p3, P4&& p4, P5&& p5, P6&& p6, P7&& p7, P8&& p8)
{
    return shared_ptr<T>(std::make_shared<T>(std::forward<P0>(p0),
                                             std::forward<P1>(p1),
                                             std::forward<P2>(p2),
                                             std::forward<P3>(p3),
                                             std::forward<P4>(p4),
                                             std::forward<P5>(p5),
                                             std::forward<P6>(p6),
                                             std::forward<P7>(p7),
                                             std::forward<P8>(p8)));
}

template <typename T,
          typename P0,
          typename P1,
          typename P2,
          typename P3,
          typename P4,
          typename P5,
          typename P6,
          typename P7,
          typename P8,
          typename P9>
shared_ptr<T> make_shared(P0&& p0, P1&& p1, P2&& p2, P3&& p3, P4&& p4, P5&& p5, P6&& p6, P7&& p7, P8&& p8, P9&& p9)
{
    return shared_ptr<T>(std::make_shared<T>(std::forward<P0>(p0),
                                             std::forward<P1>(p1),
                                             std::forward<P2>(p2),
                                             std::forward<P3>(p3),
                                             std::forward<P4>(p4),
                                             std::forward<P5>(p5),
                                             std::forward<P6>(p6),
                                             std::forward<P7>(p7),
                                             std::forward<P8>(p8),
                                             std::forward<P9>(p9)));
}

template <typename T,
          typename P0,
          typename P1,
          typename P2,
          typename P3,
          typename P4,
          typename P5,
          typename P6,
          typename P7,
          typename P8,
          typename P9,
          typename P10>
shared_ptr<T>
make_shared(P0&& p0, P1&& p1, P2&& p2, P3&& p3, P4&& p4, P5&& p5, P6&& p6, P7&& p7, P8&& p8, P9&& p9, P10&& p10)
{
    return shared_ptr<T>(std::make_shared<T>(std::forward<P0>(p0),
                                             std::forward<P1>(p1),
                                             std::forward<P2>(p2),
                                             std::forward<P3>(p3),
                                             std::forward<P4>(p4),
                                             std::forward<P5>(p5),
                                             std::forward<P6>(p6),
                                             std::forward<P7>(p7),
                                             std::forward<P8>(p8),
                                             std::forward<P9>(p9),
                                             std::forward<P10>(p10)));
}

template <typename T,
          typename P0,
          typename P1,
          typename P2,
          typename P3,
          typename P4,
          typename P5,
          typename P6,
          typename P7,
          typename P8,
          typename P9,
          typename P10,
          typename P11>
shared_ptr<T> make_shared(P0&&  p0,
                          P1&&  p1,
                          P2&&  p2,
                          P3&&  p3,
                          P4&&  p4,
                          P5&&  p5,
                          P6&&  p6,
                          P7&&  p7,
                          P8&&  p8,
                          P9&&  p9,
                          P10&& p10,
                          P11&& p11)
{
    return shared_ptr<T>(std::make_shared<T>(std::forward<P0>(p0),
                                             std::forward<P1>(p1),
                                             std::forward<P2>(p2),
                                             std::forward<P3>(p3),
                                             std::forward<P4>(p4),
                                             std::forward<P5>(p5),
                                             std::forward<P6>(p6),
                                             std::forward<P7>(p7),
                                             std::forward<P8>(p8),
                                             std::forward<P9>(p9),
                                             std::forward<P10>(p10),
                                             std::forward<P11>(p11)));
}

template <typename T>
shared_ptr<T>::shared_ptr()
    : p_(make_shared<T>())
{
}

}} // namespace caspar::spl
