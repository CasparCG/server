/*
 *          Copyright Andrey Semashev 2007 - 2010.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   if.hpp
 * \author Andrey Semashev
 * \date   12.01.2008
 *
 * The header contains implementation of a conditional formatter.
 */

#if (defined(_MSC_VER) && _MSC_VER > 1000)
#pragma once
#endif // _MSC_VER > 1000

#ifndef BOOST_LOG_FORMATTERS_IF_HPP_INCLUDED_
#define BOOST_LOG_FORMATTERS_IF_HPP_INCLUDED_

#include <boost/log/detail/prologue.hpp>
#include <boost/log/formatters/basic_formatters.hpp>

namespace boost {

namespace BOOST_LOG_NAMESPACE {

namespace formatters {

//! Conditional 'if-else' formatter
template< typename FilterT, typename ThenT, typename ElseT >
class fmt_if_else :
    public basic_formatter< typename ThenT::char_type, fmt_if_else< FilterT, ThenT, ElseT > >
{
private:
    //! Base type
    typedef basic_formatter< typename ThenT::char_type, fmt_if_else< FilterT, ThenT, ElseT > > base_type;

public:
    //! String type
    typedef typename base_type::string_type string_type;
    //! Stream type
    typedef typename base_type::ostream_type ostream_type;
    //! Log record type
    typedef typename base_type::record_type record_type;

private:
    FilterT m_Filter;
    ThenT m_Then;
    ElseT m_Else;

public:
    /*!
     * Constructor
     *
     * \param flt The condition filter
     * \param th The formatter that gets invoked if \a flt returns \c true
     * \param el The formatter that gets invoked if \a flt returns \c false
     */
    fmt_if_else(FilterT const& flt, ThenT const& th, ElseT const& el) : m_Filter(flt), m_Then(th), m_Else(el) {}

    /*!
     * Formatting operator. Applies the filter to the log record. If the filter returns \c true
     * passes the received arguments to the aggregated then-formatter. Otherwise calls else-formatter.
     *
     * \param strm A reference to the stream, where the final text of the logging record is composed
     * \param record A logging record
     */
    void operator() (ostream_type& strm, record_type const& record) const
    {
        if (m_Filter(record.attribute_values()))
            m_Then(strm, record);
        else
            m_Else(strm, record);
    }
};

//! Conditional 'if' formatter
template< typename FilterT, typename FormatterT >
class fmt_if :
    public basic_formatter< typename FormatterT::char_type, fmt_if< FilterT, FormatterT > >
{
private:
    //! Base type
    typedef basic_formatter< typename FormatterT::char_type, fmt_if< FilterT, FormatterT > > base_type;

public:
    //! String type
    typedef typename base_type::string_type string_type;
    //! Stream type
    typedef typename base_type::ostream_type ostream_type;
    //! Log record type
    typedef typename base_type::record_type record_type;

#ifndef BOOST_LOG_DOXYGEN_PASS

private:
    class else_gen
    {
        friend class fmt_if;

    private:
        FilterT m_Filter;
        FormatterT m_Formatter;

    public:
        //! Constructor
        else_gen(FilterT const& flt, FormatterT const& fmt) : m_Filter(flt), m_Formatter(fmt) {}
        //! If-else formatter generation operator
        template< typename ElseT >
        fmt_if_else< FilterT, FormatterT, ElseT > operator[] (ElseT const& el) const
        {
            return fmt_if_else< FilterT, FormatterT, ElseT >(m_Filter, m_Formatter, el);
        }
    };

public:

    //! Else-formatter generation object
    else_gen else_;

#else // BOOST_LOG_DOXYGEN_PASS

    /*!
     * Else-formatter generation object
     */
    implementation_defined else_;

#endif // BOOST_LOG_DOXYGEN_PASS

public:
    /*!
     * Constructor
     *
     * \param flt The condition filter
     * \param fmt The formatter that gets invoked if \a flt returns \c true
     */
    fmt_if(FilterT const& flt, FormatterT const& fmt) : else_(flt, fmt) {}

    /*!
     * Formatting operator. Applies the filter to the log record. If the filter returns \c true
     * passes the received arguments to the aggregated formatter. Otherwise does nothing.
     *
     * \param strm A reference to the stream, where the final text of the logging record is composed
     * \param record A logging record
     */
    void operator() (ostream_type& strm, record_type const& record) const
    {
        if (else_.m_Filter(record.attribute_values()))
            else_.m_Formatter(strm, record);
    }
};

namespace aux {

    //! Conditional formatter generator
    template< typename FilterT >
    class fmt_if_gen
    {
        FilterT m_Filter;

    public:
        explicit fmt_if_gen(FilterT const& filter) : m_Filter(filter) {}
        template< typename FormatterT >
        fmt_if< FilterT, FormatterT > operator[] (FormatterT const& fmt) const
        {
            return fmt_if< FilterT, FormatterT >(m_Filter, fmt);
        }
    };

} // namespace aux

#ifndef BOOST_LOG_DOXYGEN_PASS

//! Generator function
template< typename FilterT >
inline aux::fmt_if_gen< FilterT > if_(FilterT const& flt)
{
    return aux::fmt_if_gen< FilterT >(flt);
}

#else // BOOST_LOG_DOXYGEN_PASS

/*!
 * The function returns a conditional formatter generator object. The generator provides <tt>operator[]</tt> that can be used
 * to construct the actual formatter.
 *
 * \param flt A Boost.Log filter that represents condition of the formatter
 */
template< typename FilterT >
implementation_defined if_(FilterT const& flt);

#endif // BOOST_LOG_DOXYGEN_PASS

} // namespace formatters

} // namespace log

} // namespace boost

#endif // BOOST_LOG_FORMATTERS_IF_HPP_INCLUDED_
