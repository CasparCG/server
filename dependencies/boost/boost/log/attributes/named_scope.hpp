/*
 *          Copyright Andrey Semashev 2007 - 2010.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file
 * \author Andrey Semashev
 * \date   24.06.2007
 *
 * The header contains implementation of named scope container and an attribute that allows to
 * put the named scope to log. A number of convenience macros are also provided.
 */

#if (defined(_MSC_VER) && _MSC_VER > 1000)
#pragma once
#endif // _MSC_VER > 1000

#ifndef BOOST_LOG_ATTRIBUTES_NAMED_SCOPE_HPP_INCLUDED_
#define BOOST_LOG_ATTRIBUTES_NAMED_SCOPE_HPP_INCLUDED_

#include <ostream>
#include <memory>
#include <iterator>
#include <boost/log/detail/prologue.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/current_function.hpp>
#include <boost/mpl/if.hpp>
#ifdef BOOST_LOG_USE_WCHAR_T
#include <boost/preprocessor/cat.hpp>
#endif
#include <boost/log/utility/string_literal.hpp>
#include <boost/log/utility/unique_identifier_name.hpp>
#include <boost/log/utility/no_unused_warnings.hpp>
#include <boost/log/attributes/attribute.hpp>

#ifdef _MSC_VER
#pragma warning(push)
// non dll-interface class 'A' used as base for dll-interface class 'B'
#pragma warning(disable: 4275)
// 'm_A' : class 'A' needs to have dll-interface to be used by clients of class 'B'
#pragma warning(disable: 4251)
#endif // _MSC_VER

namespace boost {

namespace BOOST_LOG_NAMESPACE {

namespace attributes {

namespace aux {

    //! Double-linked list node
    struct named_scope_list_node
    {
        mutable named_scope_list_node* _m_pPrev;
        mutable named_scope_list_node* _m_pNext;

        named_scope_list_node() { _m_pPrev = _m_pNext = this; }
    };

} // namespace aux

/*!
 * \brief The structure contains all information about a named scope
 *
 * The named scope entries are stored as elements of \c basic_named_scope_list container, which
 * in turn can be acquired either from the \c basic_named_scope attribute value or from a thread-local
 * instance.
 */
template< typename CharT >
struct basic_named_scope_entry
    //! \cond
    : public aux::named_scope_list_node
    //! \endcond
{
    //! Character type
    typedef CharT char_type;

    /*!
     * The scope name (e.g. a function signature)
     */
    basic_string_literal< char_type > scope_name;
    /*!
     * The source file name
     */
    basic_string_literal< char_type > file_name;
    /*!
     * The line number in the source file
     */
    unsigned int line;

    /*!
     * Initializing constructor
     *
     * \post <tt>scope_name == sn && file_name == fn && line == ln</tt>
     *
     * \b Throws: Nothing.
     */
    basic_named_scope_entry(
        basic_string_literal< char_type > const& sn,
        basic_string_literal< char_type > const& fn,
        unsigned int ln) : scope_name(sn), file_name(fn), line(ln)
    {
    }
};

/*!
 * \brief The class implements the list of scopes
 *
 * The scope list provides a read-only access to a doubly-linked list of scopes.
 */
template< typename CharT >
class basic_named_scope_list
    //! \cond
    : protected std::allocator< basic_named_scope_entry< CharT > >
    //! \endcond
{
    //! Self type
    typedef basic_named_scope_list< CharT > this_type;

public:
    //! Character type
    typedef CharT char_type;
    //! Allocator type
    typedef std::allocator< basic_named_scope_entry< char_type > > allocator_type;

    //  Standard types
    typedef typename allocator_type::value_type value_type;
    typedef typename allocator_type::reference reference;
    typedef typename allocator_type::const_reference const_reference;
    typedef typename allocator_type::pointer pointer;
    typedef typename allocator_type::const_pointer const_pointer;
    typedef typename allocator_type::size_type size_type;
    typedef typename allocator_type::difference_type difference_type;

#ifndef BOOST_LOG_DOXYGEN_PASS

protected:
    //! Iterator class
#ifndef BOOST_LOG_NO_MEMBER_TEMPLATE_FRIENDS
    template< bool fConstV > class iter;
    template< bool fConstV > friend class iter;
#endif
    template< bool fConstV >
    class iter
    {
        friend class iter< !fConstV >;

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
        explicit iter(aux::named_scope_list_node* pNode) : m_pNode(pNode) {}
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
            m_pNode = m_pNode->_m_pNext;
            return *this;
        }
        iter& operator-- ()
        {
            m_pNode = m_pNode->_m_pPrev;
            return *this;
        }
        iter operator++ (int)
        {
            iter tmp(*this);
            m_pNode = m_pNode->_m_pNext;
            return tmp;
        }
        iter operator-- (int)
        {
            iter tmp(*this);
            m_pNode = m_pNode->_m_pPrev;
            return tmp;
        }

        //  Dereferencing
        pointer operator-> () const { return static_cast< pointer >(m_pNode); }
        reference operator* () const { return *static_cast< pointer >(m_pNode); }

    private:
        aux::named_scope_list_node* m_pNode;
    };

public:
    typedef iter< true > const_iterator;
    typedef iter< false > iterator;
    typedef std::reverse_iterator< const_iterator > const_reverse_iterator;
    typedef std::reverse_iterator< iterator > reverse_iterator;

#else

    /*!
     * A constant iterator to the sequence of scopes. Complies to bidirectional iterator requirements.
     */
    typedef implementation_defined const_iterator;
    /*!
     * An iterator to the sequence of scopes. Complies to bidirectional iterator requirements.
     */
    typedef implementation_defined iterator;
    /*!
     * A constant reverse iterator to the sequence of scopes. Complies to bidirectional iterator requirements.
     */
    typedef implementation_defined const_reverse_iterator;
    /*!
     * A reverse iterator to the sequence of scopes. Complies to bidirectional iterator requirements.
     */
    typedef implementation_defined reverse_iterator;

#endif // BOOST_LOG_DOXYGEN_PASS

protected:
    //! The root node of the container
    aux::named_scope_list_node m_RootNode;
    //! The size of the container
    size_type m_Size;
    //! The flag shows if the contained elements are dynamically allocated
    bool m_fNeedToDeallocate;

public:
    /*!
     * Default constructor
     *
     * \post <tt>empty() == true</tt>
     */
    basic_named_scope_list() : m_Size(0), m_fNeedToDeallocate(false) {}
    /*!
     * Copy constructor
     *
     * \post <tt>std::equal(begin(), end(), that.begin()) == true</tt>
     */
    BOOST_LOG_EXPORT basic_named_scope_list(basic_named_scope_list const& that);
    /*!
     * Destructor. Destroys the stored entries.
     */
    BOOST_LOG_EXPORT ~basic_named_scope_list();

    /*!
     * Assignment operator
     *
     * \post <tt>std::equal(begin(), end(), that.begin()) == true</tt>
     */
    basic_named_scope_list& operator= (basic_named_scope_list const& that)
    {
        if (this != &that)
        {
            basic_named_scope_list tmp(that);
            swap(tmp);
        }
        return *this;
    }

    /*!
     * \return Constant iterator to the first element of the container.
     */
    const_iterator begin() const { return const_iterator(m_RootNode._m_pNext); }
    /*!
     * \return Constant iterator to the after-the-last element of the container.
     */
    const_iterator end() const { return const_iterator(const_cast< aux::named_scope_list_node* >(&m_RootNode)); }
    /*!
     * \return Constant iterator to the last element of the container.
     */
    const_reverse_iterator rbegin() const { return const_reverse_iterator(end()); }
    /*!
     * \return Constant iterator to the before-the-first element of the container.
     */
    const_reverse_iterator rend() const { return const_reverse_iterator(begin()); }

    /*!
     * \return The number of elements in the container
     */
    size_type size() const { return m_Size; }
    /*!
     * \return true if the container is empty and false otherwise
     */
    bool empty() const { return (m_Size == 0); }

    /*!
     * Swaps two instances of the container
     */
    BOOST_LOG_EXPORT void swap(basic_named_scope_list& that);

    /*!
     * \return Last pushed scope entry
     */
    const_reference back() const { return *rbegin(); }
    /*!
     * \return First pushed scope entry
     */
    const_reference front() const { return *begin(); }
};

//! Stream output operator
template< typename CharT, typename TraitsT >
inline std::basic_ostream< CharT, TraitsT >& operator<< (
    std::basic_ostream< CharT, TraitsT >& strm, basic_named_scope_list< CharT > const& sl)
{
    typename basic_named_scope_list< CharT >::const_iterator it = sl.begin(), end = sl.end();
    if (it != end)
        strm << (it++)->scope_name;
    for (; it != end; ++it)
        strm << "->" << it->scope_name;
    return strm;
}

/*!
 * \brief A class of an attribute that holds stack of named scopes of the current thread
 *
 * The basic_named_scope attribute is essentially a hook to the thread-specific instance of
 * scope list. This means that the attribute will generate different values if get_value is
 * called in different threads. The attribute generates value with stored type
 * <tt>basic_named_scope_list< CharT ></tt>.
 *
 * The attribute class can also be used to gain access to the scope stack instance, e.g. to
 * get its copy or to push or pop a scope entry. However, it is highly not recommended to
 * maintain scope list manually. Use \c BOOST_LOG_NAMED_SCOPE or \c BOOST_LOG_FUNCTION macros instead.
 */
template< typename CharT >
class BOOST_LOG_EXPORT basic_named_scope :
    public attribute
{
public:
    //! Character type
    typedef CharT char_type;
    //! Scope names stack (the attribute value type)
    typedef basic_named_scope_list< char_type > scope_stack;
    //! Scope entry
    typedef typename scope_stack::value_type scope_entry;

    //! Sentry object class to automatically push and pop scopes
    struct sentry
    {
        //! Attribute type
        typedef basic_named_scope< char_type > named_scope_type;

        /*!
         * Constructor. Pushes the specified scope to the end of the thread-local list of scopes.
         *
         * \param sn Scope name.
         * \param fn File name, in which the scope is located.
         * \param ln Line number in the file.
         */
        sentry(
            basic_string_literal< char_type > const& sn,
            basic_string_literal< char_type > const& fn,
            unsigned int ln) : m_Entry(sn, fn, ln)
        {
            named_scope_type::push_scope(m_Entry);
        }

        /*!
         * Destructor. Removes the last pushed scope from the thread-local list of scopes.
         */
        ~sentry()
        {
            named_scope_type::pop_scope();
        }

    private:
        scope_entry m_Entry;
    };

private:
    //! Attribute implementation class
    struct implementation;

private:
    //! Pointer to the implementation
    shared_ptr< implementation > pImpl;

public:
    /*!
     * Constructor. Creates an attribute.
     */
    basic_named_scope();

    shared_ptr< attribute_value > get_value();

    /*!
     * The method pushes the scope to the back of the current thread's scope list
     *
     * \b Throws: Nothing.
     */
    static void push_scope(scope_entry const& entry);
    /*!
     * The method pops the last pushed scope from the current thread's scope list
     *
     * \b Throws: Nothing.
     */
    static void pop_scope();

    /*!
     *  \return The current thread's list of scopes
     *
     *  \note The returned reference is only valid until the current thread ends. The scopes in the
     *        returned container may change if the execution scope is changed (i.e. either \c push_scope
     *        or \c pop_scope is called). User has to copy the stack if he wants to keep it intact regardless
     *        of the execution scope.
     */
    static scope_stack const& get_scopes();
};

#if defined(BOOST_LOG_USE_CHAR)
typedef basic_named_scope_list< char > named_scope_list;        //!< Convenience typedef for narrow-character logging
typedef basic_named_scope< char > named_scope;                  //!< Convenience typedef for narrow-character logging
#endif
#if defined(BOOST_LOG_USE_WCHAR_T)
typedef basic_named_scope_list< wchar_t > wnamed_scope_list;    //!< Convenience typedef for wide-character logging
typedef basic_named_scope< wchar_t > wnamed_scope;              //!< Convenience typedef for wide-character logging
#endif

} // namespace attributes

} // namespace log

} // namespace boost

#ifdef _MSC_VER
#pragma warning(pop)
#endif // _MSC_VER

//! \cond

#define BOOST_LOG_NAMED_SCOPE_INTERNAL(char_type, var, name, file, line)\
    ::boost::log::attributes::basic_named_scope< char_type >::sentry var(name, file, line);\
    BOOST_LOG_NO_UNUSED_WARNINGS(var)

//! \endcond

#ifdef BOOST_LOG_USE_CHAR

/*!
 * Macro for scope markup. The specified scope name is pushed to the end of the current thread scope list.
 */
#define BOOST_LOG_NAMED_SCOPE(name)\
    BOOST_LOG_NAMED_SCOPE_INTERNAL(char, BOOST_LOG_UNIQUE_IDENTIFIER_NAME(_boost_log_named_scope_sentry_), name, __FILE__, __LINE__)

/*!
 * Macro for function scope markup. The scope name is constructed with help of compiler and contains current function name.
 * The scope name is pushed to the end of the current thread scope list.
 *
 * Not all compilers have support for this macro. The exact form of the scope name may vary from one compiler to another.
 */
#define BOOST_LOG_FUNCTION() BOOST_LOG_NAMED_SCOPE(BOOST_CURRENT_FUNCTION)

#endif

#ifdef BOOST_LOG_USE_WCHAR_T

/*!
 * Macro for scope markup. The specified scope name is pushed to the end of the current thread scope list.
 */
#define BOOST_LOG_WNAMED_SCOPE(name)\
    BOOST_LOG_NAMED_SCOPE_INTERNAL(wchar_t, BOOST_LOG_UNIQUE_IDENTIFIER_NAME(_boost_log_named_scope_sentry_), name, BOOST_PP_CAT(L, __FILE__), __LINE__)

/*!
 * Macro for function scope markup. The scope name is constructed with help of compiler and contains current function name.
 * The scope name is pushed to the end of the current thread scope list.
 *
 * Not all compilers have support for this macro. The exact form of the scope name may vary from one compiler to another.
 */
#define BOOST_LOG_WFUNCTION() BOOST_LOG_WFUNCTION_(BOOST_PP_CAT(L, BOOST_CURRENT_FUNCTION))

#endif

#endif // BOOST_LOG_ATTRIBUTES_NAMED_SCOPE_HPP_INCLUDED_
