/*
 *          Copyright Andrey Semashev 2007 - 2010.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   attribute_values_view.hpp
 * \author Andrey Semashev
 * \date   21.04.2007
 *
 * This header file contains definition of attribute values view. The view is constructed from
 * three attribute sets (global, thread-specific and source-specific) and contains attribute
 * values.
 */

#if (defined(_MSC_VER) && _MSC_VER > 1000)
#pragma once
#endif // _MSC_VER > 1000

#ifndef BOOST_LOG_ATTRIBUTE_VALUES_VIEW_HPP_INCLUDED_
#define BOOST_LOG_ATTRIBUTE_VALUES_VIEW_HPP_INCLUDED_

#include <memory>
#include <iterator>
#include <algorithm>
#include <boost/shared_ptr.hpp>
#include <boost/utility/addressof.hpp>
#include <boost/log/detail/prologue.hpp>
#include <boost/log/utility/slim_string.hpp>
#include <boost/log/attributes/attribute.hpp>
#include <boost/log/attributes/attribute_set.hpp>

namespace boost {

namespace BOOST_LOG_NAMESPACE {

/*!
 * \brief A view of attribute values
 *
 * Attribute values view is a read-only associative container with attribute name as a key and
 * a pointer to attribute value object as a mapped type. This is a collection of elements with unique
 * keys, that is, there can be only one attribute value with a given name in a view. With respect to
 * read-only capabilities, attribute values view is close to \c std::map.
 *
 * An instance of attribute values view can be constructed from three attribute sets and attempts to
 * accommodate values all attributes from the sets. The situation when a same-named attribute is found
 * in more than one attribute is possible. This problem is solved on construction of the view: the three
 * attribute sets have different priorities when it comes to solving conflicts.
 *
 * From the library perspective the three source attribute sets are global, thread-specific and source-specific
 * attributes, with the latter having the highest priority. This feature allows to override attributes of wider scopes
 * with the more specific ones.
 *
 * After the view construction it cannot be modified. However, for sake of performance, the attribute values
 * are not immediately acquired on the view construction. Instead, on-demand acquision is performed either on
 * iterator dereferencing or on call to the \c freeze method. Once acquired, the attribute value stays within the view
 * until its destruction. This nuance does not affect other view properties, such as size or lookup ability.
 * The logging core automatically freezes the view at the right point, so users should not be bothered.
 *
 * \note The attribute sets that were used for the view construction must not be modified or destroyed
 *       until the view is frozen. Otherwise the behavior is undefined.
 */
template< typename CharT >
class basic_attribute_values_view :
    private std::allocator< char >
{
    //! Self type
    typedef basic_attribute_values_view< CharT > this_type;

public:
    //! Character type
    typedef CharT char_type;
    //! String type
    typedef std::basic_string< char_type > string_type;
    //! Key type
    typedef basic_slim_string< char_type > key_type;
    //! Mapped attribute type
    typedef shared_ptr< attribute_value > mapped_type;
    //! Corresponding attribute set type
    typedef basic_attribute_set< char_type > attribute_set_type;

    //! Value type
    typedef std::pair< const key_type, mapped_type > value_type;
    //! Allocator type
    typedef std::allocator< value_type > allocator_type;
    //! Reference type
    typedef typename allocator_type::reference reference;
    //! Const reference type
    typedef typename allocator_type::const_reference const_reference;
    //! Pointer type
    typedef typename allocator_type::pointer pointer;
    //! Const pointer type
    typedef typename allocator_type::const_pointer const_pointer;
    //! Size type
    typedef typename allocator_type::size_type size_type;
    //! Pointer difference type
    typedef typename allocator_type::difference_type difference_type;

#ifndef BOOST_LOG_DOXYGEN_PASS

private:
    typedef std::allocator< char > internal_allocator_type;
    struct implementation;

    //! Contained node type
    struct node
    {
        value_type m_Value;
        attribute* m_pAttribute;

        node(key_type const& key, attribute* attr)
            : m_Value(key, mapped_type()), m_pAttribute(attr)
        {
        }
    };

public:
    class const_iterator;
    friend class const_iterator;
    class const_iterator
    {
    public:
        //  Standard typedefs
        typedef typename this_type::difference_type difference_type;
        typedef typename this_type::value_type value_type;
        typedef typename this_type::const_reference reference;
        typedef typename this_type::const_pointer pointer;
        typedef std::bidirectional_iterator_tag iterator_category;

    public:
        //  Constructors
        const_iterator() : m_pNode(NULL) {}
        explicit const_iterator(node* pNode) : m_pNode(pNode) {}

        //  Comparison
        bool operator== (const_iterator const& that) const { return (m_pNode == that.m_pNode); }
        bool operator!= (const_iterator const& that) const { return (m_pNode != that.m_pNode); }

        //  Modification
        const_iterator& operator++ ()
        {
            ++m_pNode;
            return *this;
        }
        const_iterator& operator-- ()
        {
            --m_pNode;
            return *this;
        }
        const_iterator operator++ (int)
        {
            const_iterator tmp(*this);
            ++m_pNode;
            return tmp;
        }
        const_iterator operator-- (int)
        {
            const_iterator tmp(*this);
            --m_pNode;
            return tmp;
        }

        //  Dereferencing
        pointer operator-> () const
        {
            freeze_element();
            return boost::addressof(m_pNode->m_Value);
        }
        reference operator* () const
        {
            freeze_element();
            return m_pNode->m_Value;
        }

        //! The method ensures that the pointed element has acquired the attribute value
        void freeze_element() const
        {
            if (!m_pNode->m_Value.second)
                m_pNode->m_Value.second = m_pNode->m_pAttribute->get_value();
        }

    private:
        //! The pointed element of the container
        node* m_pNode;
    };

#else

    /*!
     * Constant iterator type with bidirectional capabilities.
     */
    typedef implementation_defined const_iterator;

#endif // BOOST_LOG_DOXYGEN_PASS

private:
    //! Pointer to the container implementation
    implementation* m_pImpl;

public:
    /*!
     * The constructor adopts three attribute sets into the view. The \a source_attrs attributes have the greatest preference
     * when a same-named attribute is found in several sets, \a global_attrs has the least. The constructed view is not frozen.
     *
     * \param source_attrs A set of source-specific attributes.
     * \param thread_attrs A set of thread-specific attributes.
     * \param global_attrs A set of global attributes.
     */
    BOOST_LOG_EXPORT basic_attribute_values_view(
        attribute_set_type const& source_attrs,
        attribute_set_type const& thread_attrs,
        attribute_set_type const& global_attrs);

    /*!
     * Copy constructor.
     *
     * \pre The original view is frozen.
     * \post The constructed view is frozen, <tt>std::equal(begin(), end(), that.begin()) == true</tt>
     */
    BOOST_LOG_EXPORT basic_attribute_values_view(basic_attribute_values_view const& that);
    /*!
     * Destructor. Releases all referenced attribute values.
     */
    BOOST_LOG_EXPORT ~basic_attribute_values_view();

    /*!
     * Assignment operator
     *
     * \pre The original view is frozen.
     * \post The resulting view is frozen, <tt>std::equal(begin(), end(), that.begin()) == true</tt>
     */
    BOOST_LOG_EXPORT basic_attribute_values_view& operator= (basic_attribute_values_view const& that);

    /*!
     * Swaps two views
     *
     * \b Throws: Nothing.
     */
    void swap(basic_attribute_values_view& that)
    {
        std::swap(m_pImpl, that.m_pImpl);
    }

    /*!
     * \return Iterator to the first element of the view.
     */
    BOOST_LOG_EXPORT const_iterator begin() const;
    /*!
     * \return Iterator to the after-the-last element of the view.
     */
    BOOST_LOG_EXPORT const_iterator end() const;

    /*!
     * \return Number of elements in the view.
     */
    BOOST_LOG_EXPORT size_type size() const;
    /*!
     * \return true if there are no elements in the container, false otherwise.
     */
    bool empty() const { return (size() == 0); }

    /*!
     * The method finds the attribute value by name.
     *
     * \param key Attribute name.
     * \return Iterator to the found element or \c end() if the attribute with such name is not found.
     */
    const_iterator find(key_type const& key) const
    {
        return find_impl(key.data(), key.size());
    }
    /*!
     * The method finds the attribute value by name.
     *
     * \param key Attribute name.
     * \return Iterator to the found element or \c end() if the attribute with such name is not found.
     */
    const_iterator find(string_type const& key) const
    {
        return find_impl(key.data(), key.size());
    }
    /*!
     * The method finds the attribute value by name.
     *
     * \param key Attribute name. Must not be NULL, must point to a zero-terminated string.
     * \return Iterator to the found element or \c end() if the attribute with such name is not found.
     */
    const_iterator find(const char_type* key) const
    {
        typedef std::char_traits< char_type > traits_type;
        return find_impl(key, traits_type::length(key));
    }

    /*!
     * Alternative lookup syntax.
     *
     * \param key Attribute name.
     * \return A pointer to the attribute value if it is found with \a key, default-constructed mapped value otherwise.
     */
    mapped_type operator[] (key_type const& key) const
    {
        const_iterator it = this->find(key);
        if (it != this->end())
            return it->second;
        else
            return mapped_type();
    }
    /*!
     * Alternative lookup syntax.
     *
     * \param key Attribute name.
     * \return A pointer to the attribute value if it is found with \a key, default-constructed mapped value otherwise.
     */
    mapped_type operator[] (string_type const& key) const
    {
        const_iterator it = this->find(key);
        if (it != this->end())
            return it->second;
        else
            return mapped_type();
    }
    /*!
     * Alternative lookup syntax.
     *
     * \param key Attribute name. Must not be NULL, must point to a zero-terminated string.
     * \return A pointer to the attribute value if it is found with \a key, default-constructed mapped value otherwise.
     */
    mapped_type operator[] (const char_type* key) const
    {
        const_iterator it = this->find(key);
        if (it != this->end())
            return it->second;
        else
            return mapped_type();
    }

    /*!
     * The method counts the number of the attribute value occurrences in the view. Since there can be only one
     * attribute value with a particular key, the method always return 0 or 1.
     *
     * \param key Attribute name.
     * \return The number of times the attribute value is found in the container.
     */
    size_type count(key_type const& key) const { return size_type(find(key) != end()); }
    /*!
     * The method counts the number of the attribute value occurrences in the view. Since there can be only one
     * attribute value with a particular key, the method always return 0 or 1.
     *
     * \param key Attribute name.
     * \return The number of times the attribute value is found in the container.
     */
    size_type count(string_type const& key) const { return size_type(find(key) != end()); }
    /*!
     * The method counts the number of the attribute value occurrences in the view. Since there can be only one
     * attribute value with a particular key, the method always return 0 or 1.
     *
     * \param key Attribute name. Must not be NULL, must point to a zero-terminated string.
     * \return The number of times the attribute value is found in the container.
     */
    size_type count(const char_type* key) const { return size_type(find(key) != end()); }

    /*!
     * The method acquires values of all adopted attributes.
     *
     * \post The view is frozen
     */
    BOOST_LOG_EXPORT void freeze();

private:
    //! \cond

    //! Internal lookup implementation
    BOOST_LOG_EXPORT const_iterator find_impl(const char_type* key, size_type len) const;

    //! \endcond
};

/*!
 * Free swap overload
 */
template< typename CharT >
inline void swap(basic_attribute_values_view< CharT >& left, basic_attribute_values_view< CharT >& right)
{
    left.swap(right);
}

typedef basic_attribute_values_view< char > attribute_values_view;      //!< Convenience typedef for narrow-character logging
typedef basic_attribute_values_view< wchar_t > wattribute_values_view;  //!< Convenience typedef for wide-character logging

} // namespace log

} // namespace boost

#endif // BOOST_LOG_ATTRIBUTE_VALUES_VIEW_HPP_INCLUDED_
