#pragma once

#include "except.h"

#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>

#include <string>
#include <type_traits>

namespace caspar {

class param_comparer
{
    const std::wstring& lhs;

  public:
    explicit param_comparer(const std::wstring& p)
        : lhs(p)
    {
    }
    bool operator()(const std::wstring& rhs) { return boost::iequals(lhs, rhs); }
};

template <typename C>
bool contains_param(const std::wstring& name, C&& params)
{
    return std::find_if(params.begin(), params.end(), param_comparer(name)) != params.end();
}

template <typename C>
bool get_and_consume_flag(const std::wstring& flag_param, C& params)
{
    auto flag_it = std::find_if(params.begin(), params.end(), param_comparer(flag_param));
    bool flag    = false;

    if (flag_it != params.end()) {
        flag = true;
        params.erase(flag_it);
    }

    return flag;
}

template <typename C>
void replace_placeholders(const std::wstring& placeholder, const std::wstring& replacement, C&& params)
{
    for (auto& param : params)
        boost::ireplace_all(param, placeholder, replacement);
}

static std::pair<std::wstring, std::wstring> protocol_split(const std::wstring& s)
{
    size_t pos;
    if ((pos = s.find(L"://")) != std::wstring::npos) {
        return std::make_pair(s.substr(0, pos), s.substr(pos + 3));
    }

    return std::make_pair(L"", s);
}

template <typename T, typename C>
typename std::enable_if<!std::is_convertible<T, std::wstring>::value, typename std::decay<T>::type>::type
get_param(const std::wstring& name, C&& params, T fail_value = T())
{
    auto it = std::find_if(std::begin(params), std::end(params), param_comparer(name));
    if (it == params.end())
        return fail_value;

    try {
        if (++it == params.end())
            throw std::out_of_range("");

        return boost::lexical_cast<typename std::decay<T>::type>(*it);
    } catch (...) {
        CASPAR_THROW_EXCEPTION(user_error() << msg_info(L"Failed to parse param " + name)
                                            << nested_exception(std::current_exception()));
    }
}

template <typename C>
std::wstring get_param(const std::wstring& name, C&& params, const std::wstring& fail_value = L"")
{
    auto it = std::find_if(std::begin(params), std::end(params), param_comparer(name));
    if (it == params.end())
        return fail_value;

    try {
        if (++it == params.end())
            throw std::out_of_range("");

        return *it;
    } catch (...) {
        CASPAR_THROW_EXCEPTION(user_error() << msg_info(L"Failed to parse param " + name)
                                            << nested_exception(std::current_exception()));
    }
}

} // namespace caspar
