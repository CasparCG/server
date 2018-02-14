#pragma once

#include <boost/any.hpp>

#include <cstddef>
#include <cstdlib>
#include <memory>
#include <vector>

namespace caspar {

template <typename T>
class array final
{
    template <typename>
    friend class array;

  public:
    typedef T*       iterator;
    typedef const T* const_iterator;

    array() = default;

    array(std::vector<T> storage)
        : ptr_(storage.data())
        , size_(storage.size())
        , storage_(std::make_unique<boost::any>(std::move(storage)))
    {
    }

    explicit array(std::size_t size)
        : size_(size)
    {
        if (size_ > 0) {
            auto storage = std::shared_ptr<void>(std::malloc(size), std::free);
            ptr_ = reinterpret_cast<T*>(storage.get());
            storage_ = std::make_unique<boost::any>(std::move(storage));
        }
    }

    template <typename S>
    explicit array(T* ptr, std::size_t size, S&& storage)
        : ptr_(ptr)
        , size_(size)
        , storage_(std::make_unique<boost::any>(std::forward<S>(storage)))
    {
    }

    array(const array<T>&) = delete;

    array(array&& other)
        : ptr_(other.ptr_)
        , size_(other.size_)
        , storage_(std::move(other.storage_))
    {
    }

    array& operator=(const array<T>&) = delete;

    array& operator=(array&& other)
    {
        ptr_ = std::move(other.ptr_);
        size_ = std::move(other.size_);
        storage_ = std::move(other.storage_);

        return *this;
    }

    T*          begin() const { return ptr_; }
    T*          data() const { return ptr_; }
    T*          end() const { return ptr_ + size_; }
    std::size_t size() const { return size_; }

    explicit operator bool() const { return size_ > 0; };

    template <typename S>
    S* storage() const
    {
        return boost::any_cast<S>(storage_.get());
    }

  private:
    T*                          ptr_  = nullptr;
    std::size_t                 size_ = 0;
    std::unique_ptr<boost::any> storage_;
};

template <typename T>
class array<const T> final
{
  public:
    typedef const T* iterator;
    typedef const T* const_iterator;

    array() = default;

    array(std::vector<T> storage)
        : ptr_(storage.data())
        , size_(storage.size())
        , storage_(std::make_unique<boost::any>(std::move(storage)))
    {
    }

    array(std::size_t size)
        : size_(size)
    {
        if (size_ > 0) {
            auto storage = std::vector<char>(size, 0);
            ptr_ = reinterpret_cast<T*>(storage.data());
            storage_ = std::move(storage);
        }
    }

    template <typename S>
    explicit array(const T* ptr, std::size_t size, S&& storage)
        : ptr_(ptr)
        , size_(size)
        , storage_(std::make_shared<boost::any>(std::forward<S>(storage)))
    {
    }

    array(const array& other)
        : ptr_(other.ptr_)
        , size_(other.size_)
        , storage_(other.storage_)
    {
    }

    array(array<T>&& other)
        : ptr_(other.ptr_)
        , size_(other.size_)
        , storage_(std::move(other.storage_))
    {
    }

    array& operator=(array other)
    {
        other.swap(*this);
        return *this;
    }

    void swap(array& other)
    {
        std::swap(ptr_, other.ptr_);
        std::swap(size_, other.size_);
        std::swap(storage_, other.storage_);
    }

    const T*    begin() const { return ptr_; }
    const T*    data() const { return ptr_; }
    const T*    end() const { return ptr_ + size_; }
    std::size_t size() const { return size_; }

    explicit operator bool() const { return size_ > 0; }

    template <typename S>
    S* storage() const
    {
        return boost::any_cast<S>(storage_.get());
    }

  private:
    const T*                    ptr_  = nullptr;
    std::size_t                 size_ = 0;
    std::shared_ptr<boost::any> storage_;
};

} // namespace caspar

namespace std {

template <typename T>
void swap(caspar::array<const T>& lhs, caspar::array<const T>& rhs)
{
    lhs.swap(rhs);
}

} // namespace std
