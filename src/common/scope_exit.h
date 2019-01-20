#pragma once

#include "except.h"

namespace caspar {

namespace detail {
template <typename T>
class scope_exit
{
    scope_exit(const scope_exit&);
    scope_exit& operator=(const scope_exit&);

  public:
    template <typename T2>
    explicit scope_exit(T2&& func)
        : func_(std::forward<T2>(func))
        , valid_(true)
    {
    }

    scope_exit(scope_exit&& other)
        : func_(std::move(other.func_))
        , valid_(std::move(other.valid_))
    {
        other.valid_ = false;
    }

    scope_exit& operator=(scope_exit&& other)
    {
        func_  = std::move(other.func_);
        valid_ = std::move(other.valid_);

        other.valid_ = false;

        return *this;
    }

    ~scope_exit()
    {
        try {
            if (valid_)
                func_();
        } catch (...) {
            if (!std::uncaught_exception())
#pragma warning(push)
#pragma warning(disable : 4297)
                throw;
#pragma warning(pop)
            else
                CASPAR_LOG_CURRENT_EXCEPTION();
        }
    }

  private:
    T    func_;
    bool valid_;
};

class scope_exit_helper
{
};

template <typename T>
scope_exit<typename std::decay<T>::type> operator+(scope_exit_helper /*unused*/, T&& exitScope)
{
    return scope_exit<typename std::decay<T>::type>(std::forward<T>(exitScope));
}
} // namespace detail

#define _CASPAR_EXIT_SCOPE_LINENAME_CAT(name, line) name##line
#define _CASPAR_EXIT_SCOPE_LINENAME(name, line) _CASPAR_EXIT_SCOPE_LINENAME_CAT(name, line)
#define CASPAR_SCOPE_EXIT                                                                                              \
    auto _CASPAR_EXIT_SCOPE_LINENAME(EXIT, __LINE__) = ::caspar::detail::scope_exit_helper() + [&]() mutable

} // namespace caspar