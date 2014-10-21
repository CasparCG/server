/*
 *          Copyright Andrey Semashev 2007 - 2010.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   attribute_set.hpp
 * \author Andrey Semashev
 * \date   08.03.2007
 *
 * This header contains definition of the attribute set container.
 */

#if (defined(_MSC_VER) && _MSC_VER > 1000)
#pragma once
#endif // _MSC_VER > 1000

#ifndef BOOST_LOG_ATTRIBUTE_SET_HPP_INCLUDED_
#define BOOST_LOG_ATTRIBUTE_SET_HPP_INCLUDED_

#include <string>
#include <utility>
#include <iterator>
#include <algorithm>
#include <boost/shared_ptr.hpp>
#include <boost/mpl/if.hpp>
#include <boost/utility/addressof.hpp>
#include <boost/log/detail/prologue.hpp>
#include <boost/log/utility/slim_string.hpp>
#include <boost/log/attributes/attribute.hpp>

namespace boost {

namespace BOOST_LOG_NAMESPACE {

template< typename >
class basic_attribute_values_view;

/*!
 * \brief An attribute set class.
 *
 * An attribute set is an associative container with attribute name as a key and
 * pointer to the attribute as a mapped value. The container allows storing only one element for each distinct
 * key value. In most regards attribute set container provides interface similar to \c std::map. However, there are
 * differences in \c operator[] semantics and a number of optimizations with regard to iteration. Besides,
 * attribute names are stored as a read-only <tt>slim_string</tt>'s instead of \c std::string, which is also saves memory
 * and CPU time.
 */
template< typename CharT >
class basic_attribute_set
{
    friend class basic_attribute_values_view< CharT >;

    //! Self type
    typedef basic_attribute_set< CharT > this_type;

public:
    //! Character type
    typedef CharT char_type;
    //! String type
    typedef std::basic_string< char_type > string_type;
    //! Key type
    typedef basic_slim_string< char_type > key_type;
    //! Mapped attribute type
    typedef shared_ptr< attribute > mapped_type;

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
    //! Difference type
    typedef typename allocator_type::difference_type difference_type;

private:
    //! \cond

    //! Implementation
    struct implementation;
    friend struct implementation;

    //! Reference proxy object to implement \c operator[]
    class reference_proxy;
    friend class reference_proxy;
    class reference_proxy
    {
        basic_attribute_set* m_pContainer;
        const char_type* m_pKey;
        size_type m_KeyLen;

    public:
        //! Constructor
        explicit reference_proxy(basic_attribute_set* pContainer, const char_type* pKey, size_type KeyLen)
            : m_pContainer(pContainer), m_pKey(pKey), m_KeyLen(KeyLen)
        {
        }

        //! Conversion operator (would be invoked in case of reading from the container)
        operator mapped_type() const
        {
            iterator it = m_pContainer->find_impl(m_pKey, m_KeyLen);
            if (it != m_pContainer->end())
                return it->second;
            else
                return mapped_type();
        }
        //! Assignment operator (would be invoked in case of writing to the container)
        mapped_type& operator= (mapped_type const& val) const
        {
            std::pair< iterator, bool > res =
                m_pContainer->insert(key_type(m_pKey, m_KeyLen), val);
            if (!res.second)
                res.first->second = val;
            return res.first->second;
        }
    };

    //! A base class for the container nodes
    struct node_base
    {
        value_type m_Value;
        node_base* m_pPrev;
        node_base* m_pNext;

        node_base() { /* m_pPrev = m_pNext = 0; -- initialized internally in cpp */ }
        node_base(node_base const& that) : m_Value(that.m_Value) { /* m_pPrev = m_pNext = 0; -- initialized internally in cpp */ }
        node_base(key_type const& key, mapped_type const& data) : m_Value(key, data) { /* m_pPrev = m_pNext = 0; -- initialized internally in cpp */ }

    private:
        node_base& operator= (node_base const&);
    };

    //! Iterator class
#ifndef BOOST_LOG_NO_MEMBER_TEMPLATE_FRIENDS
    template< bool fConstV > class iter;
    template< bool fConstV > friend class iter;
#endif
    template< bool fConstV >
    class iter
    {
        friend class iter< !fConstV >;
        friend class basic_attribute_set< CharT >;

    public:
        //  Standard typedefs
        typedef typename this_type::difference_type difference_type;
        typedef typename this_type::value_type value_type;
        typedef typename mpl::if_c<
            fConstV,
            typename this_type::const_reference,
            typename this_type::reference
        >::type reference;
        typedef typename mpl::if_c<
            fConstV,
            typename this_type::const_pointer,
            typename this_type::pointer
        >::type pointer;
        typedef std::bidirectional_iterator_tag iterator_category;

    public:
        //  Constructors
        iter() : m_pNode(NULL) {}
        explicit iter(node_base* pNode) : m_pNode(pNode) {}
        iter(iter< false > const& that) : m_pNode(that.m_pNode) {}

        //! Assignment
        template< bool f >
        iter& operator= (iter< f > const& that)
        {
            m_pNode = that.m_pNode;
            return *this;
        }

        //  Comparison
        template< bool f >
        bool operator== (iter< f > const& that) const { return (m_pNode == that.m_pNode); }
        template< bool f >
        bool operator!= (iter< f > const& that) const { return (m_pNode != that.m_pNode); }

        //  Modification
        iter& operator++ ()
        {
            m_pNode = m_pNode->m_pNext;
            return *this;
        }
        iter& operator-- ()
        {
            m_pNode = m_pNode->m_pPrev;
            return *this;
        }
        iter operator++ (int)
        {
            iter tmp(*this);
            m_pNode = m_pNode->m_pNext;
            return tmp;
        }
        iter operator-- (int)
        {
            iter tmp(*this);
            m_pNode = m_pNode->m_pPrev;
            return tmp;
        }

        //  Dereferencing
        pointer operator-> () const { return boost::addressof(m_pNode->m_Value); }
        reference operator* () const { return m_pNode->m_Value; }

    private:
        node_base* m_pNode;
    };

    //! \endcond

public:
#ifndef BOOST_LOG_DOXYGEN_PASS
    //! Iterator type
    typedef iter< false > iterator;
    //! Const iterator type
    typedef iter< true > const_iterator;
#else
    /*!
     * Iterator type. The iterator complies to the bidirectional iterator requirements.
     */
    typedef implementation_defined iterator;
    /*!
     * Constant iterator type. The iterator complies to the bidirectional iterator requirements with read-only capabilities.
     */
    typedef implementation_defined const_iterator;
#endif // BOOST_LOG_DOXYGEN_PASS

private:
    //! Pointer to implementation
    implementation* m_pImpl;

public:
    /*!
     * Default constructor.
     *
     * \post <tt>empty() == true</tt>
     */
    BOOST_LOG_EXPORT basic_attribute_set();
    /*!
     * Copy constructor.
     *
     * \post <tt>std::equal(begin(), end(), that.begin()) == true</tt>
     */
    BOOST_LOG_EXPORT basic_attribute_set(basic_attribute_set const& that);
    /*!
     * Destructor. All stored references to attributes are released.
     */
    BOOST_LOG_EXPORT ~basic_attribute_set();

    /*!
     * Assignment operator.
     *
     * \post <tt>std::equal(begin(), end(), that.begin()) == true</tt>
     */
    BOOST_LOG_EXPORT basic_attribute_set& operator= (basic_attribute_set const& that);

    /*!
     * Swaps two instances of the container.
     *
     * \b Throws: Nothing.
     */
    void swap(basic_attribute_set& that) { std::swap(m_pImpl, that.m_pImpl); }

    /*!
     * \return Iterator to the first element of the container.
     */
    BOOST_LOG_EXPORT iterator begin();
    /*!
     * \return Iterator to the after-the-last element of the container.
     */
    BOOST_LOG_EXPORT iterator end();
    /*!
     * \return Constant iterator to the first element of the container.
     */
    BOOST_LOG_EXPORT const_iterator begin() const;
    /*!
     * \return Constant iterator to the after-the-last element of the container.
     */
    BOOST_LOG_EXPORT const_iterator end() const;

    /*!
     * \return Number of elements in the container.
     */
    BOOST_LOG_EXPORT size_type size() const;
    /*!
     * \return true if there are no elements in the container, false otherwise.
     */
    bool empty() const { return (size() == 0); }

    /*!
     * The method finds the attribute by name.
     *
     * \param key Attribute name.
     * \return Iterator to the found element or end() if the attribute with such name is not found.
     */
    iterator find(key_type const& key)
    {
        return this->find_impl(key.data(), key.size());
    }
    /*!
     * The method finds the attribute by name.
     *
     * \param key Attribute name.
     * \return Iterator to the found element or \c end() if the attribute with such name is not found.
     */
    iterator find(string_type const& key)
    {
        return this->find_impl(key.data(), key.size());
    }
    /*!
     * The method finds the attribute by name.
     *
     * \param key Attribute name. Must not be NULL, must point to a zero-terminated string.
     * \return Iterator to the found element or \c end() if the attribute with such name is not found.
     */
    iterator find(const char_type* key)
    {
        typedef std::char_traits< char_type > traits_type;
        return this->find_impl(key, traits_type::length(key));
    }
    /*!
     * The method finds the attribute by name.
     *
     * \param key Attribute name.
     * \return Iterator to the found element or \c end() if the attribute with such name is not found.
     */
    const_iterator find(key_type const& key) const
    {
        return this->find_impl(key.data(), key.size());
    }
    /*!
     * The method finds the attribute by name.
     *
     * \param key Attribute name.
     * \return Iterator to the found element or \c end() if the attribute with such name is not found.
     */
    const_iterator find(string_type const& key) const
    {
        return this->find_impl(key.data(), key.size());
    }
    /*!
     * The method finds the attribute by name.
     *
     * \param key Attribute name. Must not be NULL, must point to a zero-terminated string.
     * \return Iterator to the found element or \c end() if the attribute with such name is not found.
     */
    const_iterator find(const char_type* key) const
    {
        typedef std::char_traits< char_type > traits_type;
        return this->find_impl(key, traits_type::length(key));
    }
    /*!
     * The method counts the number of the attribute occurrences in the container. Since there can be only one
     * attribute with a particular key, the method always return 0 or 1.
     *
     * \param key Attribute name.
     * \return The number of times the attribute is found in the container.
     */
    size_type count(key_type const& key) const { return size_type(find(key) != end()); }
    /*!
     * The method counts the number of the attribute occurrences in the container. Since there can be only one
     * attribute with a particular key, the method always return 0 or 1.
     *
     * \param key Attribute name.
     * \return The number of times the attribute is found in the container.
     */
    size_type count(string_type const& key) const { return size_type(find(key) != end()); }
    /*!
     * The method counts the number of the attribute occurrences in the container. Since there can be only one
     * attribute with a particular key, the method always return 0 or 1.
     *
     * \param key Attribute name. Must not be NULL, must point to a zero-terminated string.
     * \return The number of times the attribute is found in the container.
     */
    size_type count(const char_type* key) const { return size_type(find(key) != end()); }

    /*!
     * Combined lookup/insertion operator. The operator semantics depends on the further usage of the returned reference.
     * \li If the reference is used as an assignment target, the assignment expression is equivalent to element insertion,
     *     where the element is composed of the second argument of the \c operator[] as a key and the second argument of assignment
     *     as a mapped value.
     * \li If the returned reference is used in context where a conversion to the mapped type is required,
     *     the result of the conversion is equivalent to the mapped value found with the second argument of the \c operator[] as a key,
     *     if such an element exists in the container, or a default-constructed mapped value, if an element does not exist in the
     *     container.
     *
     * \param key Attribute name.
     * \return A smart reference object of unspecified type.
     */
    reference_proxy operator[] (key_type const& key)
    {
        return reference_proxy(this, key.data(), key.size());
    }
    /*!
     * Combined lookup/insertion operator. The operator semantics depends on the further usage of the returned reference.
     * \li If the reference is used as an assignment target, the assignment expression is equivalent to element insertion,
     *     where the element is composed of the second argument of the \c operator[] as a key and the second argument of assignment
     *     as a mapped value.
     * \li If the returned reference is used in context where a conversion to the mapped type is required,
     *     the result of the conversion is equivalent to the mapped value found with the second argument of the \c operator[] as a key,
     *     if such an element exists in the container, or a default-constructed mapped value, if an element does not exist in the
     *     container.
     *
     * \param key Attribute name.
     * \return A smart reference object of unspecified type.
     */
    reference_proxy operator[] (string_type const& key)
    {
        return reference_proxy(this, key.data(), key.size());
    }
    /*!
     * Combined lookup/insertion operator. The operator semantics depends on the further usage of the returned reference.
     * \li If the reference is used as an assignment target, the assignment expression is equivalent to element insertion,
     *     where the element is composed of the second argument of the \c operator[] as a key and the second argument of assignment
     *     as a mapped value.
     * \li If the returned reference is used in context where a conversion to the mapped type is required,
     *     the result of the conversion is equivalent to the mapped value found with the second argument of the \c operator[] as a key,
     *     if such an element exists in the container, or a default-constructed mapped value, if an element does not exist in the
     *     container.
     *
     * \param key Attribute name. Must not be NULL, must point to a zero-terminated string.
     * \return A smart reference object of unspecified type.
     */
    reference_proxy operator[] (const char_type* key)
    {
        typedef std::char_traits< char_type > traits_type;
        return reference_proxy(this, key, traits_type::length(key));
    }
    /*!
     * Lookup operator
     *
     * \param key Attribute name.
     * \return If an element with the corresponding attribute name is found in the container, its mapped value
     *         is returned. Otherwise a refault-constructed mapped value is returned.
     */
    mapped_type operator[] (key_type const& key) const
    {
        const_iterator it = find(key);
        if (it != end())
            return it->second;
        else
            return mapped_type();
    }
    /*!
     * Lookup operator
     *
     * \param key Attribute name.
     * \return If an element with the corresponding attribute name is found in the container, its mapped value
     *         is returned. Otherwise a refault-constructed mapped value is returned.
     */
    mapped_type operator[] (string_type const& key) const
    {
        const_iterator it = find(key);
        if (it != end())
            return it->second;
        else
            return mapped_type();
    }
    /*!
     * Lookup operator
     *
     * \param key Attribute name. Must not be NULL, must point to a zero-terminated string.
     * \return If an element with the corresponding attribute name is found in the container, its mapped value
     *         is returned. Otherwise a refault-constructed mapped value is returned.
     */
    mapped_type operator[] (const char_type* key) const
    {
        const_iterator it = find(key);
        if (it != end())
            return it->second;
        else
            return mapped_type();
    }

    /*!
     * Insertion method
     *
     * \param key Attribute name.
     * \param data Pointer to the attribute. Must not be NULL.
     * \returns A pair of values. If second is true, the insertion succeeded and the first component points to the
     *          inserted element. Otherwise the first component points to the element that prevents insertion.
     */
    BOOST_LOG_EXPORT std::pair< iterator, bool > insert(key_type const& key, mapped_type const& data);

    /*!
     * Insertion method
     *
     * \param value An element to be inserted.
     * \returns A pair of values. If second is true, the insertion succeeded and the first component points to the
     *          inserted element. Otherwise the first component points to the element that prevents insertion.
     */
    std::pair< iterator, bool > insert(const_reference value)
    {
        return insert(value.first, value.second);
    }

    /*!
     * Mass insertion method.
     *
     * \param begin A forward iterator that points to the first element to be inserted.
     * \param end A forward iterator that points to the after-the-last element to be inserted.
     */
    template< typename FwdIteratorT >
    void insert(FwdIteratorT begin, FwdIteratorT end)
    {
        for (; begin != end; ++begin)
            insert(*begin);
    }

    /*!
     * Mass insertion method with ability to acquire iterators to the inserted elements.
     *
     * \param begin A forward iterator that points to the first element to be inserted.
     * \param end A forward iterator that points to the after-the-last element to be inserted.
     * \param out An output iterator that receives results of insertion of the elements
     */
    template< typename FwdIteratorT, typename OutputIteratorT >
    void insert(FwdIteratorT begin, FwdIteratorT end, OutputIteratorT out)
    {
        for (; begin != end; ++begin, ++out)
            *out = insert(*begin);
    }

    /*!
     * The method erases all attributes with the specified name
     *
     * \post All iterators to the erased elements become invalid.
     * \param key Attribute name.
     * \return Tne number of erased elements
     */
    BOOST_LOG_EXPORT size_type erase(key_type const& key);
    /*!
     * The method erases the specified attribute
     *
     * \post All iterators to the erased element become invalid.
     * \param it A valid iterator to the element to be erased.
     * \return Tne number of erased elements
     */
    BOOST_LOG_EXPORT void erase(iterator it);
    /*!
     * The method erases all attributes within the specified range
     *
     * \pre \a end is reachable from \a begin with a finite number of increments.
     * \post All iterators to the erased elements become invalid.
     * \param begin An iterator that points to the first element to be erased.
     * \param end An iterator that points to the after-the-last element to be erased.
     */
    BOOST_LOG_EXPORT void erase(iterator begin, iterator end);

    /*!
     * The method removes all elements from the container
     *
     * \post <tt>empty() == true</tt>
     */
    BOOST_LOG_EXPORT void clear();

private:
    //! \cond

    //! Internal lookup implementation
    BOOST_LOG_EXPORT iterator find_impl(const char_type* key, size_type len);
    //! Internal lookup implementation
    const_iterator find_impl(const char_type* key, size_type len) const
    {
        return const_iterator(const_cast< basic_attribute_set* >(this)->find_impl(key, len));
    }

    //! \endcond
};

/*!
 * Free swap overload
 */
template< typename CharT >
inline void swap(basic_attribute_set< CharT >& left, basic_attribute_set< CharT >& right)
{
    left.swap(right);
}

typedef basic_attribute_set< char > attribute_set;      //!< Convenience typedef for narrow-character logging
typedef basic_attribute_set< wchar_t > wattribute_set;  //!< Convenience typedef for wide-character logging

} // namespace log

} // namespace boost

#endif // BOOST_LOG_ATTRIBUTE_SET_HPP_INCLUDED_
