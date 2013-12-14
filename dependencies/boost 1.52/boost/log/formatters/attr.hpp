/*
 *          Copyright Andrey Semashev 2007 - 2010.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   formatters/attr.hpp
 * \author Andrey Semashev
 * \date   22.04.2007
 *
 * The header contains implementation of a generic attribute formatter.
 */

#if (defined(_MSC_VER) && _MSC_VER > 1000)
#pragma once
#endif // _MSC_VER > 1000

#ifndef BOOST_LOG_FORMATTERS_ATTR_HPP_INCLUDED_
#define BOOST_LOG_FORMATTERS_ATTR_HPP_INCLUDED_

#include <new> // std::nothrow
#include <string>
#include <boost/format.hpp>
#include <boost/parameter/binding.hpp>
#include <boost/utility/enable_if.hpp>
#include <boost/log/detail/prologue.hpp>
#include <boost/log/detail/cleanup_scope_guard.hpp>
#include <boost/log/formatters/basic_formatters.hpp>
#include <boost/log/formatters/exception_policies.hpp>
#include <boost/log/utility/type_dispatch/standard_types.hpp>
#include <boost/log/utility/attribute_value_extractor.hpp>
#include <boost/log/keywords/format.hpp>

namespace boost {

namespace BOOST_LOG_NAMESPACE {

namespace formatters {

template< typename CharT, typename AttributeValueTypesT, typename ExceptionPolicyT >
class fmt_attr;
template< typename CharT, typename AttributeValueTypesT, typename ExceptionPolicyT >
class fmt_attr_formatted;

namespace aux {

    //! Output stream operator
    template< typename StreamT >
    struct ostream_op
    {
        typedef void result_type;
        explicit ostream_op(StreamT& strm) : m_Stream(strm) {}
        template< typename T >
        void operator() (T const& value) const
        {
            m_Stream << value;
        }

    private:
        StreamT& m_Stream;
    };

    //! Boost.Format binding operator
    template< typename FormatT >
    struct format_op
    {
        typedef void result_type;
        explicit format_op(FormatT& fmt) : m_Format(fmt) {}
        template< typename T >
        void operator() (T const& value) const
        {
            m_Format % value;
        }

    private:
        FormatT& m_Format;
    };

    template< typename CharT, typename AttributeValueTypesT, typename ExceptionPolicyT, typename FormatT >
    struct make_attr_formatter_impl
    {
        typedef fmt_attr_formatted< CharT, AttributeValueTypesT, ExceptionPolicyT > type;
        typedef typename type::string_type string_type;

        template< typename ArgsT >
        static type construct(string_type const& name, ArgsT const& args)
        {
            return type(name, args[keywords::format]);
        }
    };
    template< typename CharT, typename AttributeValueTypesT, typename ExceptionPolicyT >
    struct make_attr_formatter_impl< CharT, AttributeValueTypesT, ExceptionPolicyT, void >
    {
        typedef fmt_attr< CharT, AttributeValueTypesT, ExceptionPolicyT > type;
        typedef typename type::string_type string_type;

        template< typename ArgsT >
        static type construct(string_type const& name, ArgsT const& args)
        {
            return type(name);
        }
    };

    template< typename CharT, typename AttributeValueTypesT, typename ExceptionPolicyT, typename ArgsT >
    struct make_attr_formatter :
        public make_attr_formatter_impl<
            CharT,
            AttributeValueTypesT,
            ExceptionPolicyT,
            typename parameter::binding<
                ArgsT,
                keywords::tag::format,
                void
            >::type
        >
    {
    };

} // namespace aux

/*!
 * \brief Generic attribute value formatter
 *
 * The \c fmt_attr formatter attempts to convert the attribute value to a string by
 * putting it into a stream. The final representation of the value is totally defined by the
 * \c operator<< overload implementation.
 *
 * The formatter can either accept a single attribute value type or a sequence of types.
 */
template< typename CharT, typename AttributeValueTypesT, typename ExceptionPolicyT >
class fmt_attr :
    public basic_formatter< CharT, fmt_attr< CharT, AttributeValueTypesT, ExceptionPolicyT > >
{
    //! Base type
    typedef basic_formatter< CharT, fmt_attr< CharT, AttributeValueTypesT, ExceptionPolicyT > > base_type;

public:
    //! Character type
    typedef typename base_type::char_type char_type;
    //! String type
    typedef typename base_type::string_type string_type;
    //! Stream type
    typedef typename base_type::ostream_type ostream_type;
    //! Log record type
    typedef typename base_type::record_type record_type;

private:
    //! Attribute value extractor
    attribute_value_extractor< char_type, AttributeValueTypesT > m_Extractor;

public:
    /*!
     * Constructor
     *
     * \param name Attribute name
     */
    explicit fmt_attr(string_type const& name) : m_Extractor(name) {}

    /*!
     * Formatting operator. Puts the attribute with the specified on construction name from
     * \a attrs into the \a strm stream.
     *
     * \param strm A reference to the stream, where the final text of the logging record is composed
     * \param record A logging record
     */
    void operator() (ostream_type& strm, record_type const& record) const
    {
        aux::ostream_op< ostream_type > op(strm);
        if (!m_Extractor(record.attribute_values(), op))
            ExceptionPolicyT::on_attribute_value_not_found(__FILE__, __LINE__);
    }
};

#ifdef BOOST_LOG_USE_CHAR

/*!
 * Formatter generator. By default the formatter will support all standard types.
 */
inline fmt_attr<
    char,
    make_default_attribute_types< char >::type,
    throw_policy
> attr(std::basic_string< char > const& name)
{
    return fmt_attr< char, make_default_attribute_types< char >::type, throw_policy >(name);
}
/*!
 * Formatter generator with ability to specify an exact attribute value type(s)
 */
template< typename AttributeValueTypesT >
inline fmt_attr<
    char,
    AttributeValueTypesT,
    throw_policy
> attr(std::basic_string< char > const& name)
{
    return fmt_attr< char, AttributeValueTypesT, throw_policy >(name);
}

/*!
 * Formatter generator. By default the formatter will support all standard types.
 * The formatter will not throw if the attribute value is not found in the record being formatted.
 * Instead, no output will be produced for this attribute value.
 */
inline fmt_attr<
    char,
    make_default_attribute_types< char >::type,
    no_throw_policy
> attr(std::basic_string< char > const& name, std::nothrow_t const&)
{
    return fmt_attr< char, make_default_attribute_types< char >::type, no_throw_policy >(name);
}
/*!
 * Formatter generator with ability to specify an exact attribute value type(s).
 * The formatter will not throw if the attribute value is not found in the record being formatted.
 * Instead, no output will be produced for this attribute value.
 */
template< typename AttributeValueTypesT >
inline fmt_attr<
    char,
    AttributeValueTypesT,
    no_throw_policy
> attr(std::basic_string< char > const& name, std::nothrow_t const&)
{
    return fmt_attr< char, AttributeValueTypesT, no_throw_policy >(name);
}

#endif

#ifdef BOOST_LOG_USE_WCHAR_T

/*!
 * Formatter generator. By default the formatter will support all standard types.
 */
inline fmt_attr<
    wchar_t,
    make_default_attribute_types< wchar_t >::type,
    throw_policy
> attr(std::basic_string< wchar_t > const& name)
{
    return fmt_attr< wchar_t, make_default_attribute_types< wchar_t >::type, throw_policy >(name);
}
/*!
 * Formatter generator with ability to specify an exact attribute value type(s)
 */
template< typename AttributeValueTypesT >
inline fmt_attr<
    wchar_t,
    AttributeValueTypesT,
    throw_policy
> attr(std::basic_string< wchar_t > const& name)
{
    return fmt_attr< wchar_t, AttributeValueTypesT, throw_policy >(name);
}

/*!
 * Formatter generator. By default the formatter will support all standard types.
 * The formatter will not throw if the attribute value is not found in the record being formatted.
 * Instead, no output will be produced for this attribute value.
 */
inline fmt_attr<
    wchar_t,
    make_default_attribute_types< wchar_t >::type,
    no_throw_policy
> attr(std::basic_string< wchar_t > const& name, std::nothrow_t const&)
{
    return fmt_attr< wchar_t, make_default_attribute_types< wchar_t >::type, no_throw_policy >(name);
}
/*!
 * Formatter generator with ability to specify an exact attribute value type(s).
 * The formatter will not throw if the attribute value is not found in the record being formatted.
 * Instead, no output will be produced for this attribute value.
 */
template< typename AttributeValueTypesT >
inline fmt_attr<
    wchar_t,
    AttributeValueTypesT,
    no_throw_policy
> attr(std::basic_string< wchar_t > const& name, std::nothrow_t const&)
{
    return fmt_attr< wchar_t, AttributeValueTypesT, no_throw_policy >(name);
}

#endif


/*!
 * \brief Generic attribute value formatter
 *
 * The \c fmt_attr_formatted formatter converts the attribute value to a string by
 * passing it to a Boost.Format formatter. The formatter is initialized with a
 * format string that may specify the exact representation of the formatted data.
 *
 * The formatter can either accept a single attribute value type or a sequence of types.
 */
template< typename CharT, typename AttributeValueTypesT, typename ExceptionPolicyT >
class fmt_attr_formatted :
    public basic_formatter< CharT, fmt_attr_formatted< CharT, AttributeValueTypesT, ExceptionPolicyT > >
{
    //! Base type
    typedef basic_formatter< CharT, fmt_attr_formatted< CharT, AttributeValueTypesT, ExceptionPolicyT > > base_type;

public:
    //! Character type
    typedef typename base_type::char_type char_type;
    //! String type
    typedef typename base_type::string_type string_type;
    //! Stream type
    typedef typename base_type::ostream_type ostream_type;
    //! Boost.Format object type
    typedef basic_format< char_type > format_type;
    //! Log record type
    typedef typename base_type::record_type record_type;

private:
    //! Attribute value extractor
    attribute_value_extractor< char_type, AttributeValueTypesT > m_Extractor;
    //! Formatter object
    mutable format_type m_Formatter;

public:
    /*!
     * Constructor with attribute name and format string initialization
     *
     * \param name Attribute name
     * \param fmt Format string. Must be compatible with Boost.Format and contain a single placeholder.
     *        The placeholder must be compatible with all attribute value types specified in \c AttributeValueTypesT
     */
    explicit fmt_attr_formatted(string_type const& name, string_type const& fmt) : m_Extractor(name), m_Formatter(fmt) {}

    /*!
     * Formatting operator. Formats the attribute with the specified on construction name from
     * \a attrs and puts the result into the \a strm stream.
     *
     * \param strm A reference to the stream, where the final text of the logging record is composed
     * \param record A logging record
     */
    void operator() (ostream_type& strm, record_type const& record) const
    {
        boost::log::aux::cleanup_guard< format_type > _(m_Formatter);
        aux::format_op< format_type > op(m_Formatter);
        if (!m_Extractor(record.attribute_values(), op))
            ExceptionPolicyT::on_attribute_value_not_found(__FILE__, __LINE__);
        strm << m_Formatter;
    }
};

#ifdef BOOST_LOG_USE_CHAR

/*!
 * Formatter generator. By default the formatter will support all standard types.
 */
inline fmt_attr_formatted<
    char,
    make_default_attribute_types< char >::type,
    throw_policy
> attr(std::basic_string< char > const& name, std::basic_string< char > const& fmt)
{
    return fmt_attr_formatted< char, make_default_attribute_types< char >::type, throw_policy >(name, fmt);
}
/*!
 * Formatter generator with ability to specify an exact attribute value type(s)
 */
template< typename AttributeValueTypesT >
inline fmt_attr_formatted<
    char,
    AttributeValueTypesT,
    throw_policy
> attr(std::basic_string< char > const& name, std::basic_string< char > const& fmt)
{
    return fmt_attr_formatted< char, AttributeValueTypesT, throw_policy >(name, fmt);
}

/*!
 * Formatter generator. By default the formatter will support all standard types.
 */
template< typename ArgsT >
inline typename lazy_enable_if<
    parameter::aux::is_tagged_argument< ArgsT >,
    aux::make_attr_formatter<
        char,
        make_default_attribute_types< char >::type,
        throw_policy,
        ArgsT
    >
>::type attr(std::basic_string< char > const& name, ArgsT const& fmt)
{
    typedef aux::make_attr_formatter<
        char,
        make_default_attribute_types< char >::type,
        throw_policy,
        ArgsT
    > maker;
    return maker::construct(name, fmt);
}
/*!
 * Formatter generator with ability to specify an exact attribute value type(s)
 */
template< typename AttributeValueTypesT, typename ArgsT >
inline typename lazy_enable_if<
    parameter::aux::is_tagged_argument< ArgsT >,
    aux::make_attr_formatter<
        char,
        AttributeValueTypesT,
        throw_policy,
        ArgsT
    >
>::type attr(std::basic_string< char > const& name, ArgsT const& fmt)
{
    typedef aux::make_attr_formatter<
        char,
        AttributeValueTypesT,
        throw_policy,
        ArgsT
    > maker;
    return maker::construct(name, fmt);
}

/*!
 * Formatter generator. By default the formatter will support all standard types.
 * The formatter will not throw if the attribute value is not found in the record being formatted.
 * Instead, no output will be produced for this attribute value.
 */
inline fmt_attr_formatted<
    char,
    make_default_attribute_types< char >::type,
    no_throw_policy
> attr(std::basic_string< char > const& name, std::basic_string< char > const& fmt, std::nothrow_t const&)
{
    return fmt_attr_formatted< char, make_default_attribute_types< char >::type, no_throw_policy >(name, fmt);
}
/*!
 * Formatter generator with ability to specify an exact attribute value type(s).
 * The formatter will not throw if the attribute value is not found in the record being formatted.
 * Instead, no output will be produced for this attribute value.
 */
template< typename AttributeValueTypesT >
inline fmt_attr_formatted<
    char,
    AttributeValueTypesT,
    no_throw_policy
> attr(std::basic_string< char > const& name, std::basic_string< char > const& fmt, std::nothrow_t const&)
{
    return fmt_attr_formatted< char, AttributeValueTypesT, no_throw_policy >(name, fmt);
}

/*!
 * Formatter generator. By default the formatter will support all standard types.
 * The formatter will not throw if the attribute value is not found in the record being formatted.
 * Instead, no output will be produced for this attribute value.
 */
template< typename ArgsT >
inline typename lazy_enable_if<
    parameter::aux::is_tagged_argument< ArgsT >,
    aux::make_attr_formatter<
        char,
        make_default_attribute_types< char >::type,
        no_throw_policy,
        ArgsT
    >
>::type attr(std::basic_string< char > const& name, ArgsT const& fmt, std::nothrow_t const&)
{
    typedef aux::make_attr_formatter<
        char,
        make_default_attribute_types< char >::type,
        no_throw_policy,
        ArgsT
    > maker;
    return maker::construct(name, fmt);
}
/*!
 * Formatter generator with ability to specify an exact attribute value type(s).
 * The formatter will not throw if the attribute value is not found in the record being formatted.
 * Instead, no output will be produced for this attribute value.
 */
template< typename AttributeValueTypesT, typename ArgsT >
inline typename lazy_enable_if<
    parameter::aux::is_tagged_argument< ArgsT >,
    aux::make_attr_formatter<
        char,
        AttributeValueTypesT,
        no_throw_policy,
        ArgsT
    >
>::type attr(std::basic_string< char > const& name, ArgsT const& fmt, std::nothrow_t const&)
{
    typedef aux::make_attr_formatter<
        char,
        AttributeValueTypesT,
        no_throw_policy,
        ArgsT
    > maker;
    return maker::construct(name, fmt);
}

#endif

#ifdef BOOST_LOG_USE_WCHAR_T

/*!
 * Formatter generator. By default the formatter will support all standard types.
 */
inline fmt_attr_formatted<
    wchar_t,
    make_default_attribute_types< wchar_t >::type,
    throw_policy
> attr(std::basic_string< wchar_t > const& name, std::basic_string< wchar_t > const& fmt)
{
    return fmt_attr_formatted< wchar_t, make_default_attribute_types< wchar_t >::type, throw_policy >(name, fmt);
}
/*!
 * Formatter generator with ability to specify an exact attribute value type(s)
 */
template< typename AttributeValueTypesT >
inline fmt_attr_formatted<
    wchar_t,
    AttributeValueTypesT,
    throw_policy
> attr(std::basic_string< wchar_t > const& name, std::basic_string< wchar_t > const& fmt)
{
    return fmt_attr_formatted< wchar_t, AttributeValueTypesT, throw_policy >(name, fmt);
}

/*!
 * Formatter generator. By default the formatter will support all standard types.
 */
template< typename ArgsT >
inline typename lazy_enable_if<
    parameter::aux::is_tagged_argument< ArgsT >,
    aux::make_attr_formatter<
        wchar_t,
        make_default_attribute_types< wchar_t >::type,
        throw_policy,
        ArgsT
    >
>::type attr(std::basic_string< wchar_t > const& name, ArgsT const& fmt)
{
    typedef aux::make_attr_formatter<
        wchar_t,
        make_default_attribute_types< wchar_t >::type,
        throw_policy,
        ArgsT
    > maker;
    return maker::construct(name, fmt);
}
/*!
 * Formatter generator with ability to specify an exact attribute value type(s)
 */
template< typename AttributeValueTypesT, typename ArgsT >
inline typename lazy_enable_if<
    parameter::aux::is_tagged_argument< ArgsT >,
    aux::make_attr_formatter<
        wchar_t,
        AttributeValueTypesT,
        throw_policy,
        ArgsT
    >
>::type attr(std::basic_string< wchar_t > const& name, ArgsT const& fmt)
{
    typedef aux::make_attr_formatter<
        wchar_t,
        AttributeValueTypesT,
        throw_policy,
        ArgsT
    > maker;
    return maker::construct(name, fmt);
}

/*!
 * Formatter generator. By default the formatter will support all standard types.
 * The formatter will not throw if the attribute value is not found in the record being formatted.
 * Instead, no output will be produced for this attribute value.
 */
inline fmt_attr_formatted<
    wchar_t,
    make_default_attribute_types< wchar_t >::type,
    no_throw_policy
> attr(std::basic_string< wchar_t > const& name, std::basic_string< wchar_t > const& fmt, std::nothrow_t const&)
{
    return fmt_attr_formatted< wchar_t, make_default_attribute_types< wchar_t >::type, no_throw_policy >(name, fmt);
}
/*!
 * Formatter generator with ability to specify an exact attribute value type(s).
 * The formatter will not throw if the attribute value is not found in the record being formatted.
 * Instead, no output will be produced for this attribute value.
 */
template< typename AttributeValueTypesT >
inline fmt_attr_formatted<
    wchar_t,
    AttributeValueTypesT,
    no_throw_policy
> attr(std::basic_string< wchar_t > const& name, std::basic_string< wchar_t > const& fmt, std::nothrow_t const&)
{
    return fmt_attr_formatted< wchar_t, AttributeValueTypesT, no_throw_policy >(name, fmt);
}

/*!
 * Formatter generator. By default the formatter will support all standard types.
 * The formatter will not throw if the attribute value is not found in the record being formatted.
 * Instead, no output will be produced for this attribute value.
 */
template< typename ArgsT >
inline typename lazy_enable_if<
    parameter::aux::is_tagged_argument< ArgsT >,
    aux::make_attr_formatter<
        wchar_t,
        make_default_attribute_types< wchar_t >::type,
        no_throw_policy,
        ArgsT
    >
>::type attr(std::basic_string< wchar_t > const& name, ArgsT const& fmt, std::nothrow_t const&)
{
    typedef aux::make_attr_formatter<
        wchar_t,
        make_default_attribute_types< wchar_t >::type,
        no_throw_policy,
        ArgsT
    > maker;
    return maker::construct(name, fmt);
}
/*!
 * Formatter generator with ability to specify an exact attribute value type(s).
 * The formatter will not throw if the attribute value is not found in the record being formatted.
 * Instead, no output will be produced for this attribute value.
 */
template< typename AttributeValueTypesT, typename ArgsT >
inline typename lazy_enable_if<
    parameter::aux::is_tagged_argument< ArgsT >,
    aux::make_attr_formatter<
        wchar_t,
        AttributeValueTypesT,
        no_throw_policy,
        ArgsT
    >
>::type attr(std::basic_string< wchar_t > const& name, ArgsT const& fmt, std::nothrow_t const&)
{
    typedef aux::make_attr_formatter<
        wchar_t,
        AttributeValueTypesT,
        no_throw_policy,
        ArgsT
    > maker;
    return maker::construct(name, fmt);
}

#endif

} // namespace formatters

} // namespace log

} // namespace boost

#endif // BOOST_LOG_FORMATTERS_ATTR_HPP_INCLUDED_
