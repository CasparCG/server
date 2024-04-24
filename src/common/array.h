#pragma once

#include <any>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <vector>

namespace caspar {

template <typename T>
class array final
{
    template <typename>
    friend class array;

  public:
    using iterator       = T*;
    using const_iterator = const T*;

    array() = default;

    explicit array(std::size_t size)
        : size_(size)
    {
        if (size_ > 0) {
            auto storage = std::shared_ptr<void>(std::malloc(size), std::free);
            ptr_         = reinterpret_cast<T*>(storage.get());
            std::memset(ptr_, 0, size_);
            storage_ = std::make_shared<std::any>(std::move(storage));
        }
    }

    array(std::vector<T> other)
    {
        auto storage = std::make_shared<std::vector<T>>(std::move(other));
        ptr_         = storage->data();
        size_        = storage->size();
        storage_     = std::make_shared<std::any>(std::move(storage));
    }

    template <typename S>
    explicit array(T* ptr, std::size_t size, S&& storage)
        : ptr_(ptr)
        , size_(size)
        , storage_(std::make_shared<std::any>(std::forward<S>(storage)))
    {
    }

    array(const array<T>&) = delete;

    array(array&& other)
        : ptr_(other.ptr_)
        , size_(other.size_)
        , storage_(std::move(other.storage_))
    {
        other.ptr_  = nullptr;
        other.size_ = 0;
    }

    array& operator=(const array<T>&) = delete;

    array& operator=(array&& other)
    {
        ptr_     = std::move(other.ptr_);
        size_    = std::move(other.size_);
        storage_ = std::move(other.storage_);

        return *this;
    }

    // Explicitly make a copy of the array, with shared backing storage
    array<T> clone() const {
        caspar::array<T> cloned;
        cloned.ptr_ = ptr_;
        cloned.size_ = size_;
        cloned.storage_ = storage_;
        return cloned;
    }

    T*          begin() const { return ptr_; }
    T*          data() const { return ptr_; }
    T*          end() const { return ptr_ + size_; }
    std::size_t size() const { return size_; }

    explicit operator bool() const { return size_ > 0; };

    template <typename S>
    S* storage() const
    {
        return std::any_cast<S>(storage_.get());
    }

  private:
    T*                        ptr_  = nullptr;
    std::size_t               size_ = 0;
    std::shared_ptr<std::any> storage_;
};

template <typename T>
class array<const T> final
{
  public:
    using iterator       = const T*;
    using const_iterator = const T*;

    array() = default;

    array(std::size_t size)
        : size_(size)
    {
        if (size_ > 0) {
            auto storage = std::shared_ptr<void>(std::malloc(size), std::free);
            ptr_         = reinterpret_cast<T*>(storage.get());
            std::memset(ptr_, 0, size_);
            storage_ = std::make_shared<std::any>(storage);
        }
    }

    array(const std::vector<T>& other)
    {
        auto storage = std::make_shared<std::vector<T>>(std::move(other));
        ptr_         = storage->data();
        size_        = storage->size();
        storage_     = std::make_shared<std::any>(std::move(storage));
    }

    template <typename S>
    explicit array(const T* ptr, std::size_t size, S&& storage)
        : ptr_(ptr)
        , size_(size)
        , storage_(std::make_shared<std::any>(std::forward<S>(storage)))
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
        , storage_(other.storage_)
    {
        other.ptr_     = nullptr;
        other.size_    = 0;
        other.storage_ = nullptr;
    }

    array& operator=(const array& other)
    {
        ptr_     = other.ptr_;
        size_    = other.size_;
        storage_ = other.storage_;
        return *this;
    }

    const T*    begin() const { return ptr_; }
    const T*    data() const { return ptr_; }
    const T*    end() const { return ptr_ + size_; }
    std::size_t size() const { return size_; }

    explicit operator bool() const { return size_ > 0; }

    template <typename S>
    S* storage() const
    {
        return std::any_cast<S>(storage_.get());
    }

  private:
    const T*                  ptr_  = nullptr;
    std::size_t               size_ = 0;
    std::shared_ptr<std::any> storage_;
};

} // namespace caspar

namespace std {

template <typename T>
void swap(caspar::array<const T>& lhs, caspar::array<const T>& rhs)
{
    lhs.swap(rhs);
}

} // namespace std
