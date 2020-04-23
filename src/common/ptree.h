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

#include "except.h"

#include <boost/algorithm/string/replace.hpp>
#include <boost/iterator/iterator_facade.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/range/adaptor/transformed.hpp>

#include <map>

namespace caspar {

struct ptree_exception : virtual user_error
{
};

static std::string to_xpath(std::string path)
{
    path.insert(path.begin(), '/');
    boost::replace_all(path, "<xmlattr>.", "@");
    boost::replace_all(path, ".", "/");
    return path;
}

template <typename T, typename Ptree>
T ptree_get(const Ptree& ptree, const typename Ptree::key_type& path)
{
    try {
        return ptree.template get<T>(path);
    } catch (boost::property_tree::ptree_bad_path&) {
        CASPAR_THROW_EXCEPTION(ptree_exception() << msg_info("No such element: " + to_xpath(u8(path))));
    } catch (const boost::property_tree::ptree_bad_data& e) {
        CASPAR_THROW_EXCEPTION(ptree_exception() << msg_info(e.what()));
    }
}

template <typename T, typename Ptree>
T ptree_get_value(const Ptree& ptree)
{
    try {
        return ptree.template get_value<T>();
    } catch (const boost::property_tree::ptree_bad_data& e) {
        CASPAR_THROW_EXCEPTION(ptree_exception() << msg_info(e.what()));
    }
}

template <typename Ptree>
const Ptree& ptree_get_child(const Ptree& ptree, const typename Ptree::key_type& path)
{
    try {
        return ptree.get_child(path);
    } catch (boost::property_tree::ptree_bad_path&) {
        CASPAR_THROW_EXCEPTION(ptree_exception() << msg_info("No such element: " + to_xpath(u8(path))));
    }
}

template <typename Ptree>
class scope_aware_ptree_child_range
{
    const Ptree& child_;

    using type = std::pair<const typename Ptree::key_type, Ptree>;

  public:
    class scoped_const_iterator
        : public boost::iterator_facade<scoped_const_iterator, type const, boost::forward_traversal_tag>
    {
        typename Ptree::const_iterator wrapped_;

      public:
        explicit scoped_const_iterator(typename Ptree::const_iterator it)
            : wrapped_(std::move(it))
        {
        }

        void increment() { ++wrapped_; }

        bool equal(const scoped_const_iterator& other) const { return wrapped_ == other.wrapped_; }

        const type& dereference() const { return *wrapped_; }
    };

    using iterator       = scoped_const_iterator;
    using const_iterator = scoped_const_iterator;

    scope_aware_ptree_child_range(const Ptree& parent, const typename Ptree::key_type& path)
        : child_(ptree_get_child(parent, path))
    {
    }

    scoped_const_iterator begin() const { return scoped_const_iterator(child_.begin()); }

    scoped_const_iterator end() const { return scoped_const_iterator(child_.end()); }
};

template <typename Key>
struct iterate_children_tag
{
    Key val;

    explicit iterate_children_tag(Key val_)
        : val(std::move(val_))
    {
    }
};

using witerate_children = iterate_children_tag<std::wstring>;
using iterate_children  = iterate_children_tag<std::string>;

template <typename Ptree>
scope_aware_ptree_child_range<Ptree> operator|(const Ptree& ptree, iterate_children_tag<typename Ptree::key_type> path)
{
    return scope_aware_ptree_child_range<Ptree>(ptree, path.val);
}

template <typename Ptree>
struct basic_scoped_element_translator
{
    mutable std::map<typename Ptree::key_type, int> by_name;

    using result_type = const std::pair<const typename Ptree::key_type, Ptree>&;

    result_type operator()(result_type pair) const { return pair; }
};

template <typename Ptree>
struct element_context_iteration_tag
{
};
static element_context_iteration_tag<boost::property_tree::wptree> welement_context_iteration;
static element_context_iteration_tag<boost::property_tree::ptree>  element_context_iteration;

template <typename Range, typename Ptree>
auto operator|(const Range& rng, element_context_iteration_tag<Ptree> tag)
    -> decltype(rng | boost::adaptors::transformed(basic_scoped_element_translator<Ptree>()))
{
    return rng | boost::adaptors::transformed(basic_scoped_element_translator<Ptree>());
}

template <typename Key, typename Ptree, typename Str>
void ptree_verify_element_name(const std::pair<const Key, Ptree>& elem, const Str& expected)
{
    if (elem.first != expected)
        CASPAR_THROW_EXCEPTION(ptree_exception()
                               << msg_info("Expected element named " + u8(expected) + ". Was " + u8(elem.first)));
}

} // namespace caspar
