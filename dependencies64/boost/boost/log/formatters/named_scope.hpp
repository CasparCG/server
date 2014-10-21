/*
 *          Copyright Andrey Semashev 2007 - 2010.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   formatters/named_scope.hpp
 * \author Andrey Semashev
 * \date   26.11.2007
 *
 * The header contains implementation of a named scope formatter.
 */

#ifndef BOOST_LOG_FORMATTERS_NAMED_SCOPE_HPP_INCLUDED_
#define BOOST_LOG_FORMATTERS_NAMED_SCOPE_HPP_INCLUDED_

#include <new> // std::nothrow
#include <string>
#include <iterator>
#include <algorithm>
#include <boost/limits.hpp>
#include <boost/preprocessor/iteration/iterate.hpp>
#include <boost/preprocessor/repetition/enum_params.hpp>
#include <boost/preprocessor/repetition/enum_binary_params.hpp>
#include <boost/log/detail/prologue.hpp>
#include <boost/log/attributes/named_scope.hpp>
#include <boost/log/formatters/basic_formatters.hpp>
#include <boost/log/formatters/exception_policies.hpp>
#include <boost/log/utility/attribute_value_extractor.hpp>
#include <boost/log/keywords/delimiter.hpp>
#include <boost/log/keywords/depth.hpp>
#include <boost/log/keywords/iteration.hpp>

namespace boost {

namespace BOOST_LOG_NAMESPACE {

namespace formatters {

//! Scope iteration directions
enum scope_iteration_direction
{
    forward,    //!< Iterate through scopes from outermost to innermost
    reverse     //!< Iterate through scopes from innermost to outermost
};

/*!
 * \brief Named scope attribute formatter
 *
 * The formatter iterates through the list of scopes and puts each one into the resulting stream.
 * The formatter supports customizing the iteration direction, depth and delimiter between the scopes.
 */
template< typename CharT, typename ExceptionPolicyT >
class fmt_named_scope :
    public basic_formatter< CharT, fmt_named_scope< CharT, ExceptionPolicyT > >
{
private:
    //! Base type
    typedef basic_formatter< CharT, fmt_named_scope< CharT, ExceptionPolicyT > > base_type;

public:
    //! Character type
    typedef typename base_type::char_type char_type;
    //! String type
    typedef typename base_type::string_type string_type;
    //! Stream type
    typedef typename base_type::ostream_type ostream_type;
    //! Log record type
    typedef typename base_type::record_type record_type;

    //! Scope stack container type
    typedef typename attributes::basic_named_scope< char_type >::scope_stack scope_stack;

private:
#ifndef BOOST_LOG_DOXYGEN_PASS

    //! A simple call forwarder
    struct binder;
    friend struct binder;
    struct binder
    {
        typedef void result_type;
        explicit binder(const fmt_named_scope* pthis, ostream_type& strm) : m_pThis(pthis), m_Strm(strm) {}
        void operator() (scope_stack const& scopes) const
        {
            m_pThis->format(m_Strm, scopes);
        }

    private:
        const fmt_named_scope* m_pThis;
        ostream_type& m_Strm;
    };

#endif // BOOST_LOG_DOXYGEN_PASS

private:
    //! Attribute value extractor
    attribute_value_extractor< char_type, scope_stack > m_Extractor;
    //! Scope delimiter
    const string_type m_ScopeDelimiter;
    //! Number of scopes to output
    const typename scope_stack::size_type m_MaxScopes;
    //! Scope iteration direction
    const scope_iteration_direction m_IterationDirection;

public:
    /*!
     * Constructor
     *
     * \param name Attribute name
     * \param delimiter Scope delimiter string
     * \param max_scopes Maximum scope iteration depth
     * \param direction Scope iteration direction
     */
    template< typename T1, typename T2 >
    fmt_named_scope(
        T1 const& name,
        T2 const& delimiter,
        typename scope_stack::size_type max_scopes,
        scope_iteration_direction direction
    ) :
        m_Extractor(name),
        m_ScopeDelimiter(delimiter),
        m_MaxScopes(max_scopes),
        m_IterationDirection(direction)
    {
    }

    /*!
     * Formatting operator. Acquires the scope list attribute value with the name
     * specified on the formatter construction from \a record and puts its contents
     * into the \a strm stream.
     *
     * \param strm A reference to the stream, where the final text of the logging record is composed
     * \param record A logging record
     */
    void operator() (ostream_type& strm, record_type const& record) const
    {
        // Extract the value and pass on to the implementation
        binder receiver(this, strm);
        if (!m_Extractor(record.attribute_values(), receiver))
            ExceptionPolicyT::on_attribute_value_not_found(__FILE__, __LINE__);
    }

private:
    //! The function performs formatting of the extracted scope stack
    void format(ostream_type& strm, scope_stack const& scopes) const
    {
        typename scope_stack::size_type const scopes_to_iterate = (std::min)(m_MaxScopes, scopes.size());
        if (m_IterationDirection == formatters::forward)
        {
            // Iterating through scopes in forward direction
            typename scope_stack::const_iterator it = scopes.end(), end = it;
            std::advance(it, -static_cast< typename scope_stack::difference_type >(scopes_to_iterate));

            if (it != end)
            {
                if (it != scopes.begin())
                    strm << "..." << m_ScopeDelimiter;

                strm << it->scope_name;
                for (++it; it != end; ++it)
                    strm << m_ScopeDelimiter << it->scope_name;
            }
        }
        else
        {
            // Iterating through scopes in reverse direction
            typename scope_stack::const_reverse_iterator it = scopes.rbegin(), end = it;
            std::advance(end, static_cast< typename scope_stack::difference_type >(scopes_to_iterate));

            if (it != end)
            {
                strm << it->scope_name;
                for (++it; it != end; ++it)
                    strm << m_ScopeDelimiter << it->scope_name;

                if (it != scopes.rend())
                    strm << m_ScopeDelimiter << "...";
            }
        }
    }
};

namespace aux {

    //! Auxiliary traits to acquire correct default delimiter depending on the character type
    template< typename CharT >
    struct default_scope_delimiter;

#ifdef BOOST_LOG_USE_CHAR
    template< >
    struct default_scope_delimiter< char >
    {
        static const char* forward() { return "->"; }
        static const char* reverse() { return "<-"; }
    };
#endif
#ifdef BOOST_LOG_USE_WCHAR_T
    template< >
    struct default_scope_delimiter< wchar_t >
    {
        static const wchar_t* forward() { return L"->"; }
        static const wchar_t* reverse() { return L"<-"; }
    };
#endif

    //! Auxiliary function to construct formatter from the complete set of arguments
    template< typename ExceptionPolicyT, typename CharT, typename ArgsT >
    fmt_named_scope< CharT, ExceptionPolicyT > named_scope(const CharT* name, ArgsT const& args)
    {
        typedef fmt_named_scope< CharT, ExceptionPolicyT > fmt_named_scope_t;

        scope_iteration_direction direction = args[keywords::iteration | formatters::forward];
        const CharT* default_delimiter =
            (direction == formatters::forward ? default_scope_delimiter< CharT >::forward() : default_scope_delimiter< CharT >::reverse());

        return fmt_named_scope_t(
            name,
            args[keywords::delimiter | default_delimiter],
            args[keywords::depth | (std::numeric_limits< std::size_t >::max)()],
            direction);
    }
    //! Auxiliary function to construct formatter from the complete set of arguments
    template< typename ExceptionPolicyT, typename CharT, typename ArgsT >
    fmt_named_scope< CharT, ExceptionPolicyT > named_scope(std::basic_string< CharT > const& name, ArgsT const& args)
    {
        typedef fmt_named_scope< CharT, ExceptionPolicyT > fmt_named_scope_t;

        scope_iteration_direction direction = args[keywords::iteration | formatters::forward];
        const CharT* default_delimiter =
            (direction == formatters::forward ? default_scope_delimiter< CharT >::forward() : default_scope_delimiter< CharT >::reverse());

        return fmt_named_scope_t(
            name,
            args[keywords::delimiter | default_delimiter],
            args[keywords::depth | (std::numeric_limits< std::size_t >::max)()],
            direction);
    }

} // namespace aux

#ifndef BOOST_LOG_DOXYGEN_PASS

#ifdef BOOST_LOG_USE_CHAR

//! Formatter generator
inline fmt_named_scope< char, throw_policy > named_scope(const char* name)
{
    return fmt_named_scope< char, throw_policy >(
        name, "->", (std::numeric_limits< std::size_t >::max)(), formatters::forward);
}
//! Formatter generator
inline fmt_named_scope< char, throw_policy > named_scope(std::basic_string< char > const& name)
{
    return fmt_named_scope< char, throw_policy >(
        name, "->", (std::numeric_limits< std::size_t >::max)(), formatters::forward);
}

//! Formatter generator
inline fmt_named_scope< char, no_throw_policy > named_scope(const char* name, std::nothrow_t const&)
{
    return fmt_named_scope< char, no_throw_policy >(
        name, "->", (std::numeric_limits< std::size_t >::max)(), formatters::forward);
}
//! Formatter generator
inline fmt_named_scope< char, no_throw_policy > named_scope(std::basic_string< char > const& name, std::nothrow_t const&)
{
    return fmt_named_scope< char, no_throw_policy >(
        name, "->", (std::numeric_limits< std::size_t >::max)(), formatters::forward);
}

#   define BOOST_LOG_ITERATION_CHAR_TYPE char

#   define BOOST_PP_FILENAME_1 <boost/log/formatters/named_scope.hpp>
#   define BOOST_PP_ITERATION_LIMITS (1, 3)
#   include BOOST_PP_ITERATE()

#   undef BOOST_LOG_ITERATION_CHAR_TYPE

#endif // BOOST_LOG_USE_CHAR

#ifdef BOOST_LOG_USE_WCHAR_T

//! Formatter generator
inline fmt_named_scope< wchar_t, throw_policy > named_scope(const wchar_t* name)
{
    return fmt_named_scope< wchar_t, throw_policy >(
        name, L"->", (std::numeric_limits< std::size_t >::max)(), formatters::forward);
}
//! Formatter generator
inline fmt_named_scope< wchar_t, throw_policy > named_scope(std::basic_string< wchar_t > const& name)
{
    return fmt_named_scope< wchar_t, throw_policy >(
        name, L"->", (std::numeric_limits< std::size_t >::max)(), formatters::forward);
}

//! Formatter generator
inline fmt_named_scope< wchar_t, no_throw_policy > named_scope(const wchar_t* name, std::nothrow_t const&)
{
    return fmt_named_scope< wchar_t, no_throw_policy >(
        name, L"->", (std::numeric_limits< std::size_t >::max)(), formatters::forward);
}
//! Formatter generator
inline fmt_named_scope< wchar_t, no_throw_policy > named_scope(std::basic_string< wchar_t > const& name, std::nothrow_t const&)
{
    return fmt_named_scope< wchar_t, no_throw_policy >(
        name, L"->", (std::numeric_limits< std::size_t >::max)(), formatters::forward);
}

#   define BOOST_LOG_ITERATION_CHAR_TYPE wchar_t

#   define BOOST_PP_FILENAME_1 <boost/log/formatters/named_scope.hpp>
#   define BOOST_PP_ITERATION_LIMITS (1, 3)
#   include BOOST_PP_ITERATE()

#   undef BOOST_LOG_ITERATION_CHAR_TYPE

#endif // BOOST_LOG_USE_WCHAR_T

#else // BOOST_LOG_DOXYGEN_PASS

/*!
 * Formatter generator. Construct the named scope formatter with the specified formatting parameters.
 *
 * \param name Attribute name
 * \param args An optional set of named parameters. Supported parameters:
 *             \li \c delimiter - a string that is used to delimit the formatted scope names. Default: "->" or "<-", depending on the iteration direction.
 *             \li \c iteration - iteration direction. Default: forward.
 *             \li \c depth - iteration depth. Default: unlimited.
 */
template< typename CharT, typename... ArgsT >
fmt_named_scope< CharT, throw_policy > named_scope(
    std::basic_string< CharT > const& name, ArgsT... const& args);

/*!
 * Formatter generator. Construct the named scope formatter with the specified formatting parameters.
 * The formatter will not throw if the attribute value is not found in the record being formatted.
 * Instead, no output will be produced for this attribute value.
 *
 * \param name Attribute name
 * \param args An optional set of named parameters. Supported parameters:
 *             \li \c delimiter - a string that is used to delimit the formatted scope names. Default: "->" or "<-", depending on the iteration direction.
 *             \li \c iteration - iteration direction. Default: forward.
 *             \li \c depth - iteration depth. Default: unlimited.
 */
template< typename CharT, typename... ArgsT >
fmt_named_scope< CharT, no_throw_policy > named_scope(
    std::basic_string< CharT > const& name, ArgsT... const& args, std::nothrow_t const&);

#endif // BOOST_LOG_DOXYGEN_PASS

} // namespace formatters

} // namespace log

} // namespace boost

#endif // BOOST_LOG_FORMATTERS_NAMED_SCOPE_HPP_INCLUDED_

#ifdef BOOST_PP_IS_ITERATING

//! Formatter generator
template< BOOST_PP_ENUM_PARAMS(BOOST_PP_ITERATION(), typename ArgT) >
inline fmt_named_scope< BOOST_LOG_ITERATION_CHAR_TYPE, throw_policy > named_scope(
    const BOOST_LOG_ITERATION_CHAR_TYPE* name,
    BOOST_PP_ENUM_BINARY_PARAMS(BOOST_PP_ITERATION(), ArgT, const& arg)
)
{
    return aux::named_scope< throw_policy >(name, (BOOST_PP_ENUM_PARAMS(BOOST_PP_ITERATION(), arg)));
}

//! Formatter generator
template< BOOST_PP_ENUM_PARAMS(BOOST_PP_ITERATION(), typename ArgT) >
inline fmt_named_scope< BOOST_LOG_ITERATION_CHAR_TYPE, throw_policy > named_scope(
    std::basic_string< BOOST_LOG_ITERATION_CHAR_TYPE > const& name,
    BOOST_PP_ENUM_BINARY_PARAMS(BOOST_PP_ITERATION(), ArgT, const& arg)
)
{
    return aux::named_scope< throw_policy >(name, (BOOST_PP_ENUM_PARAMS(BOOST_PP_ITERATION(), arg)));
}

//! Formatter generator
template< BOOST_PP_ENUM_PARAMS(BOOST_PP_ITERATION(), typename ArgT) >
inline fmt_named_scope< BOOST_LOG_ITERATION_CHAR_TYPE, no_throw_policy > named_scope(
    const BOOST_LOG_ITERATION_CHAR_TYPE* name,
    BOOST_PP_ENUM_BINARY_PARAMS(BOOST_PP_ITERATION(), ArgT, const& arg),
    std::nothrow_t const&
)
{
    return aux::named_scope< no_throw_policy >(name, (BOOST_PP_ENUM_PARAMS(BOOST_PP_ITERATION(), arg)));
}

//! Formatter generator
template< BOOST_PP_ENUM_PARAMS(BOOST_PP_ITERATION(), typename ArgT) >
inline fmt_named_scope< BOOST_LOG_ITERATION_CHAR_TYPE, no_throw_policy > named_scope(
    std::basic_string< BOOST_LOG_ITERATION_CHAR_TYPE > const& name,
    BOOST_PP_ENUM_BINARY_PARAMS(BOOST_PP_ITERATION(), ArgT, const& arg),
    std::nothrow_t const&
)
{
    return aux::named_scope< no_throw_policy >(name, (BOOST_PP_ENUM_PARAMS(BOOST_PP_ITERATION(), arg)));
}

#endif // BOOST_PP_IS_ITERATING
