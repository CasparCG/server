/*
 *          Copyright Andrey Semashev 2007 - 2010.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   record.hpp
 * \author Andrey Semashev
 * \date   09.03.2009
 *
 * This header contains a logging record class definition.
 */

#if (defined(_MSC_VER) && _MSC_VER > 1000)
#pragma once
#endif // _MSC_VER > 1000

#ifndef BOOST_LOG_CORE_RECORD_HPP_INCLUDED_
#define BOOST_LOG_CORE_RECORD_HPP_INCLUDED_

#include <string>
#include <boost/assert.hpp>
#include <boost/intrusive_ptr.hpp>
#include <boost/log/detail/prologue.hpp>
#include <boost/log/detail/unspecified_bool.hpp>
#include <boost/log/utility/intrusive_ref_counter.hpp>
#include <boost/log/attributes/attribute_values_view.hpp>

#ifdef _MSC_VER
#pragma warning(push)
 // non dll-interface class 'A' used as base for dll-interface class 'B'
#pragma warning(disable: 4275)
#endif // _MSC_VER

namespace boost {

namespace BOOST_LOG_NAMESPACE {

template< typename >
class basic_core;
template< typename >
class basic_record;
template< typename >
class basic_attribute_values_view;

/*!
 * \brief A logging record handle class
 *
 * This class is used to identify a log record in calls to the logging core. Consider using
 * the \c basic_record wrapper class for user-scope needs.
 */
class record_handle
{
    template< typename >
    friend class basic_core;
    template< typename >
    friend class basic_record;

private:
    //! Pointer to the log record data
    intrusive_ptr< intrusive_ref_counter > m_pData;

public:
    /*!
     * Equality comparison
     *
     * \param that Comparand
     * \return \c true if both <tt>*this</tt> and \a that identify the same log record or do not
     *         identify any record, \c false otherwise.
     */
    bool operator== (record_handle const& that) const
    {
        return m_pData == that.m_pData;
    }
    /*!
     * Inequality comparison
     *
     * \param that Comparand
     * \return <tt>!(*this == that)</tt>
     */
    bool operator!= (record_handle const& that) const
    {
        return !operator== (that);
    }

    /*!
     * Conversion to an unspecified boolean type
     *
     * \return \c true, if the handle identifies a log record, \c false, if the handle is not valid
     */
    BOOST_LOG_OPERATOR_UNSPECIFIED_BOOL()

    /*!
     * Inverted conversion to an unspecified boolean type
     *
     * \return \c false, if the handle identifies a log record, \c true, if the handle is not valid
     */
    bool operator! () const
    {
        return !m_pData;
    }

    /*!
     * Swaps two handles
     *
     * \param that Another handle to swap with
     * <b>Throws:</b> Nothing
     */
    void swap(record_handle& that)
    {
        m_pData.swap(that.m_pData);
    }

    /*!
     * Resets the log record handle. If there are no other handles left, the log record is closed
     * and all resources referenced by the record are released.
     *
     * \post <tt>!*this == true</tt>
     */
    void reset()
    {
        m_pData.reset();
    }

    /*!
     * Returns a non-<tt>NULL</tt> pointer if the handle is valid, <tt>NULL</tt> otherwise.
     */
    void* get() const
    {
        return m_pData.get();
    }

#ifndef BOOST_LOG_DOXYGEN_PASS
private:
    explicit record_handle(intrusive_ref_counter* p) : m_pData(p)
    {
    }
#endif // BOOST_LOG_DOXYGEN_PASS
};

/*!
 * A free-standing swap function overload for \c record_handle
 */
inline void swap(record_handle& left, record_handle& right)
{
    left.swap(right);
}


/*!
 * \brief Logging record class
 *
 * The logging record incapsulates all information related to a single logging statement,
 * in particular, attribute values view and the log message string.
 */
template< typename CharT >
class basic_record
{
    friend class basic_core< CharT >;

public:
    //! Character type
    typedef CharT char_type;
    //! Logging core type
    typedef basic_core< char_type > core_type;
    //! String type to be used as a message text holder
    typedef std::basic_string< char_type > string_type;
    //! Attribute values view type
    typedef basic_attribute_values_view< char_type > values_view_type;

#ifndef BOOST_LOG_DOXYGEN_PASS
private:
    //! Publicly available record data
    struct public_data :
        public intrusive_ref_counter
    {
        //! Shows if the record has already been detached from thread
        bool m_Detached;
        //! Attribute values view
        values_view_type m_AttributeValues;
        //! Record message
        string_type m_Message;

        //! Constructor from the attribute sets
        explicit public_data(values_view_type const& values) :
            m_Detached(false),
            m_AttributeValues(values)
        {
        }
    };

    //! Private data
    struct private_data;
    friend struct private_data;

private:
    //! A pointer to the log record data
    intrusive_ptr< public_data > m_pData;

#endif // BOOST_LOG_DOXYGEN_PASS

public:
    /*!
     * Default constructor. Creates an empty record that is equivalent to the invalid record handle.
     *
     * \post <tt>!*this == true</tt>
     */
    basic_record() {}

    /*!
     * Conversion from a record handle. Adopts the record referenced by the handle.
     *
     * \pre The handle, if valid, have been issued by the logging core with the same character type as the record being constructed.
     * \post <tt>this->handle() == rec</tt>
     * \param rec The record handle being adopted
     */
    explicit basic_record(record_handle const& rec) :
#ifdef NDEBUG
        m_pData(static_cast< public_data* >(rec.m_pData.get()))
#else
        m_pData(dynamic_cast< public_data* >(rec.m_pData.get()))
#endif // NDEBUG
    {
#ifndef NDEBUG
        // In debug builds we check that the handle was created by the proper core instance
        if (rec)
            BOOST_ASSERT(!!m_pData);
#endif // NDEBUG
    }

    /*!
     * Destructor. Destroys the record, releases any sinks and attribute values that were involved in processing this record.
     */
    ~basic_record() {}

    /*!
     * \return The handle to the record
     */
    record_handle handle() const
    {
        return record_handle(m_pData.get());
    }

    /*!
     * \return A reference to the set of attribute values attached to this record
     *
     * \pre <tt>!!*this</tt>
     */
    values_view_type const& attribute_values() const
    {
        return m_pData->m_AttributeValues;
    }

    /*!
     * \return A reference to the log record message string
     *
     * \pre <tt>!!*this</tt>
     */
    string_type& message()
    {
        return m_pData->m_Message;
    }
    /*!
     * \return A reference to the log record message string
     *
     * \pre <tt>!!*this</tt>
     */
    string_type const& message() const
    {
        return m_pData->m_Message;
    }

    /*!
     * Equality comparison
     *
     * \param that Comparand
     * \return \c true if both <tt>*this</tt> and \a that identify the same log record or do not
     *         identify any record, \c false otherwise.
     */
    bool operator== (basic_record const& that) const
    {
        return m_pData == that.m_pData;
    }
    /*!
     * Inequality comparison
     *
     * \param that Comparand
     * \return <tt>!(*this == that)</tt>
     */
    bool operator!= (basic_record const& that) const
    {
        return !operator== (that);
    }

    /*!
     * Conversion to an unspecified boolean type
     *
     * \return \c true, if the <tt>*this</tt> identifies a log record, \c false, if the <tt>*this</tt> is not valid
     */
    BOOST_LOG_OPERATOR_UNSPECIFIED_BOOL()

    /*!
     * Inverted conversion to an unspecified boolean type
     *
     * \return \c false, if the <tt>*this</tt> identifies a log record, \c true, if the <tt>*this</tt> is not valid
     */
    bool operator! () const
    {
        return !m_pData;
    }

    /*!
     * Swaps two handles
     *
     * \param that Another record to swap with
     * <b>Throws:</b> Nothing
     */
    void swap(basic_record& that)
    {
        m_pData.swap(that.m_pData);
    }

    /*!
     * Resets the log record handle. If there are no other handles left, the log record is closed
     * and all resources referenced by the record are released.
     *
     * \post <tt>!*this == true</tt>
     */
    void reset()
    {
        m_pData.reset();
    }

    /*!
     * The function ensures that the log record does not depend on any thread-specific data.
     *
     * \pre <tt>!!*this</tt>
     */
    void detach_from_thread()
    {
        if (!m_pData->m_Detached)
        {
            typename values_view_type::const_iterator
                it = m_pData->m_AttributeValues.begin(),
                end = m_pData->m_AttributeValues.end();
            for (; it != end; ++it)
            {
                // Yep, a bit hackish. I'll need a better backdoor to do it gracefully.
                it->second->detach_from_thread().swap(
                    const_cast< typename values_view_type::mapped_type& >(it->second));
            }

            m_pData->m_Detached = true;
        }
    }
};

/*!
 * A free-standing swap function overload for \c basic_record
 */
template< typename CharT >
inline void swap(basic_record< CharT >& left, basic_record< CharT >& right)
{
    left.swap(right);
}


#ifdef BOOST_LOG_USE_CHAR
typedef basic_record< char > record;        //!< Convenience typedef for narrow-character logging
#endif
#ifdef BOOST_LOG_USE_WCHAR_T
typedef basic_record< wchar_t > wrecord;    //!< Convenience typedef for wide-character logging
#endif

} // namespace log

} // namespace boost

#ifdef _MSC_VER
#pragma warning(pop)
#endif // _MSC_VER

#endif // BOOST_LOG_CORE_RECORD_HPP_INCLUDED_
