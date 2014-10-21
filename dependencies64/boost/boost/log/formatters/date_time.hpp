/*
 *          Copyright Andrey Semashev 2007 - 2010.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   date_time.hpp
 * \author Andrey Semashev
 * \date   11.03.2008
 *
 * The header contains implementation of date and time-related formatters.
 */

#ifndef BOOST_LOG_FORMATTERS_DATE_TIME_HPP_INCLUDED_
#define BOOST_LOG_FORMATTERS_DATE_TIME_HPP_INCLUDED_

#include <new> // std::nothrow
#include <string>
#include <memory>
#include <locale>
#include <ostream>
#include <iterator>
#include <boost/limits.hpp>
#include <boost/assert.hpp>
#include <boost/compatibility/cpp_c_headers/ctime>
#include <boost/compatibility/cpp_c_headers/cmath>
#include <boost/utility/addressof.hpp>
#include <boost/range/iterator_range.hpp>
#include <boost/algorithm/string/find.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/date_time/time.hpp>
#include <boost/date_time/date.hpp>
#include <boost/date_time/period.hpp>
#include <boost/date_time/time_facet.hpp>
#include <boost/date_time/date_facet.hpp>
#include <boost/date_time/compiler_config.hpp>
#include <boost/date_time/gregorian/conversion.hpp> // work around the problem of to_tm overload resolution in Boost.DateTime facets
#include <boost/date_time/local_time/conversion.hpp> // work around the problem of to_tm overload resolution in Boost.DateTime facets
#include <boost/date_time/posix_time/conversion.hpp>
#include <boost/date_time/posix_time/posix_time_types.hpp>
#include <boost/parameter/parameters.hpp>
#include <boost/mpl/placeholders.hpp>
#include <boost/type_traits/is_convertible.hpp>
#include <boost/preprocessor/cat.hpp>
#include <boost/preprocessor/comma_if.hpp>
#include <boost/preprocessor/iteration/iterate.hpp>
#include <boost/preprocessor/repetition/enum_params.hpp>
#include <boost/preprocessor/repetition/enum_binary_params.hpp>
#include <boost/preprocessor/seq/size.hpp>
#include <boost/preprocessor/seq/for_each_i.hpp>
#include <boost/log/detail/prologue.hpp>
#include <boost/log/detail/cleanup_scope_guard.hpp>
#include <boost/log/detail/attachable_sstream_buf.hpp>
#include <boost/log/formatters/basic_formatters.hpp>
#include <boost/log/formatters/exception_policies.hpp>
#include <boost/log/keywords/format.hpp>
#include <boost/log/keywords/unit_format.hpp>
#include <boost/log/utility/string_literal.hpp>
#include <boost/log/utility/attribute_value_extractor.hpp>
#include <boost/log/utility/type_dispatch/date_time_types.hpp>

namespace boost {

namespace BOOST_LOG_NAMESPACE {

namespace formatters {

namespace aux {

    //! A special base class that defines the default format strings
    template< typename >
    struct date_time_format_defaults;

#ifdef BOOST_LOG_USE_CHAR
    template< >
    struct date_time_format_defaults< char >
    {
        //! Character type
        typedef char char_type;
        //! String literal type
        typedef basic_string_literal< char_type > string_literal_type;

        static string_literal_type default_date_format() { return str_literal("%Y-%b-%d"); }
        static string_literal_type default_time_format() { return str_literal("%H:%M:%S.%f"); }
        static string_literal_type default_date_time_format() { return str_literal("%Y-%b-%d %H:%M:%S.%f"); }
        static string_literal_type default_time_duration_format() { return str_literal("%-%H:%M:%S.%f"); }
        static string_literal_type default_time_period_format() { return str_literal("[%begin% - %last%]"); }

        static string_literal_type time_period_begin_placeholder() { return str_literal("%begin%"); }
        static string_literal_type time_period_end_placeholder() { return str_literal("%end%"); }
        static string_literal_type time_period_last_placeholder() { return str_literal("%last%"); }
    };
#endif
#ifdef BOOST_LOG_USE_WCHAR_T
    template< >
    struct date_time_format_defaults< wchar_t >
    {
        //! Character type
        typedef wchar_t char_type;
        //! String literal type
        typedef basic_string_literal< char_type > string_literal_type;

        static string_literal_type default_date_format() { return str_literal(L"%Y-%b-%d"); }
        static string_literal_type default_time_format() { return str_literal(L"%H:%M:%S.%f"); }
        static string_literal_type default_date_time_format() { return str_literal(L"%Y-%b-%d %H:%M:%S.%f"); }
        static string_literal_type default_time_duration_format() { return str_literal(L"%-%H:%M:%S.%f"); }
        static string_literal_type default_time_period_format() { return str_literal(L"[%begin% - %last%]"); }

        static string_literal_type time_period_begin_placeholder() { return str_literal(L"%begin%"); }
        static string_literal_type time_period_end_placeholder() { return str_literal(L"%end%"); }
        static string_literal_type time_period_last_placeholder() { return str_literal(L"%last%"); }
    };
#endif

    //! Base class for formatters
    template< typename CharT >
    class date_time_formatter_base :
        protected date_time_format_defaults< CharT >
    {
    public:
        //! Character type
        typedef typename date_time_format_defaults< CharT >::char_type char_type;
        //! String type
        typedef std::basic_string< char_type > string_type;
        //! Stream buffer type
        typedef boost::log::aux::basic_ostringstreambuf< char_type > ostreambuf_type;
        //! Stream type
        typedef std::basic_ostream< char_type > ostream_type;

    protected:
        //! Format specifier
        const string_type m_Format;
        //! Formatting buffer string
        string_type m_Buffer;
        //! Stream buffer
        ostreambuf_type m_StreamBuf;
        //! Formatting stream
        ostream_type m_Stream;

    public:
        //! Constructor with format setup
        explicit date_time_formatter_base(const char_type* pFormat)
            : m_Format(pFormat), m_StreamBuf(m_Buffer), m_Stream(&m_StreamBuf)
        {
        }
        //! Constructor with format setup
        explicit date_time_formatter_base(string_type const& Format)
            : m_Format(Format), m_StreamBuf(m_Buffer), m_Stream(&m_StreamBuf)
        {
        }
        //! Copy constructor
        date_time_formatter_base(date_time_formatter_base const& that)
            : m_Format(that.m_Format), m_StreamBuf(m_Buffer), m_Stream(&m_StreamBuf)
        {
        }

        //! Cleanup method
        void clear()
        {
            m_Stream.clear();
            m_StreamBuf.clear();
            m_Buffer.clear();
        }
        //! Returns the formatted string
        string_type const& get() const
        {
            return m_Buffer;
        }

    protected:
        //! The method puts the date/time object into stream
        template< typename FacetT, typename T >
        void to_stream(T const& value)
        {
            std::locale loc = m_Stream.getloc();
            if (!std::has_facet< FacetT >(loc))
            {
                // Add the formatting facet
                std::auto_ptr< FacetT > facet(new FacetT(m_Format.c_str()));
                m_Stream.imbue(std::locale(loc, facet.get()));
                facet.release();
                loc = m_Stream.getloc();
            }

            // Perform formatting
            std::ostreambuf_iterator< char_type > osb_it(m_Stream);
            std::use_facet< FacetT >(loc).put(osb_it, m_Stream, m_Stream.fill(), value);
        }

    private:
        //  Assignment prohibited
        date_time_formatter_base& operator= (date_time_formatter_base const& that);
    };

    //! Time point formatting object
    template< typename CharT >
    class basic_date_time_formatter :
        public date_time_formatter_base< CharT >
    {
        //! Base type
        typedef date_time_formatter_base< CharT > base_type;

        //  A number of internal tags to detect the proper facet to use
        struct boost_time_tag {};
        struct boost_date_tag {};
        struct unsupported_type_tag {};

    public:
        //! Function object return type
        typedef void result_type;
        //! Character type
        typedef typename base_type::char_type char_type;
        //! String type
        typedef typename base_type::string_type string_type;

    public:
        //! Default constructor
        basic_date_time_formatter() : base_type(base_type::default_date_time_format().c_str()) {}
        //! Constructor with format setup
        template< typename ArgsT >
        explicit basic_date_time_formatter(ArgsT const& args) :
            base_type(args[keywords::format | base_type::default_date_time_format().str()])
        {
        }
        //! Constructor with format setup
        explicit basic_date_time_formatter(string_type const& fmt) : base_type(fmt)
        {
        }
        //! Copy constructor
        basic_date_time_formatter(basic_date_time_formatter const& that) :
            base_type(static_cast< base_type const& >(that))
        {
        }

        //! Formatting method for time_t
        void operator()(std::time_t value)
        {
            std::tm converted;
#ifdef BOOST_DATE_TIME_HAS_REENTRANT_STD_FUNCTIONS
            gmtime_r(&value, &converted);
#else
            posix_time::ptime psx = posix_time::from_time_t(value);
            converted = posix_time::to_tm(psx);
#endif
            (*this)(converted);
        }
        //! Formatting method for tm
        void operator()(std::tm const& value)
        {
            std::ostreambuf_iterator< char_type > osb_it(this->m_Stream);
            const char_type* const pFormat = this->m_Format.c_str();
            std::use_facet< std::time_put< char_type > >(this->m_Stream.getloc()).put(
                osb_it,
                this->m_Stream,
                this->m_Stream.fill(),
                &value,
                pFormat,
                pFormat + this->m_Format.size());
            this->m_Stream.flush();
        }
        //! Formatting method for Boost.DateTime date object
        template< typename T >
        void operator()(T const& value)
        {
            to_stream_dispatch(value, tag_of(boost::addressof(value)));
            this->m_Stream.flush();
        }

    private:
        template< typename T >
        void to_stream_dispatch(T const& value, boost_time_tag const&)
        {
            typedef boost::date_time::time_facet< T, char_type > facet_t;
            this->BOOST_NESTED_TEMPLATE to_stream< facet_t >(value);
        }
        template< typename T >
        void to_stream_dispatch(T const& value, boost_date_tag const&)
        {
            typedef boost::date_time::date_facet< T, char_type > facet_t;
            this->BOOST_NESTED_TEMPLATE to_stream< facet_t >(value);
        }
        template< typename T >
        void to_stream_dispatch(T const& value, unsupported_type_tag const&)
        {
            // Call a user-defined function to format the unknown type of date or time
            boost_log_format_date_time(
                this->m_Stream, value, static_cast< string_type const& >(this->m_Format));
        }

        template< typename T, typename TimeSystemT >
        static boost_time_tag tag_of(boost::date_time::base_time< T, TimeSystemT > const*)
        {
            return boost_time_tag();
        }
        template< typename T, typename CalendarT, typename DurationT >
        static boost_date_tag tag_of(boost::date_time::date< T, CalendarT, DurationT > const*)
        {
            return boost_date_tag();
        }
        static unsupported_type_tag tag_of(...)
        {
            return unsupported_type_tag();
        }
    };

    //! Date formatting object
    template< typename CharT >
    class basic_date_formatter :
        public basic_date_time_formatter< CharT >
    {
        //! Base type
        typedef basic_date_time_formatter< CharT > base_type;

    public:
        //! String type
        typedef typename base_type::string_type string_type;

    public:
        //! Default constructor
        basic_date_formatter() :
            base_type(base_type::default_date_format().str())
        {
        }
        //! Constructor with format setup
        template< typename ArgsT >
        explicit basic_date_formatter(ArgsT const& args) :
            base_type(string_type(args[keywords::format | base_type::default_date_format().str()]))
        {
        }
        //! Copy constructor
        basic_date_formatter(basic_date_formatter const& that) :
            base_type(static_cast< base_type const& >(that))
        {
        }
    };

    //! Time formatting object
    template< typename CharT >
    class basic_time_formatter :
        public basic_date_time_formatter< CharT >
    {
        //! Base type
        typedef basic_date_time_formatter< CharT > base_type;

    public:
        //! String type
        typedef typename base_type::string_type string_type;

    public:
        //! Default constructor
        basic_time_formatter() :
            base_type(base_type::default_time_format().str())
        {
        }
        //! Constructor with format setup
        template< typename ArgsT >
        explicit basic_time_formatter(ArgsT const& args) :
            base_type(string_type(args[keywords::format | base_type::default_time_format().str()]))
        {
        }
        //! Copy constructor
        basic_time_formatter(basic_time_formatter const& that) :
            base_type(static_cast< base_type const& >(that))
        {
        }
    };

    //! Time period formatting object
    template< typename CharT >
    class basic_time_duration_formatter :
        public date_time_formatter_base< CharT >
    {
        //! Base type
        typedef date_time_formatter_base< CharT > base_type;

        //  A number of internal tags to detect the proper facet to use
        struct boost_posix_time_duration_tag {};
        struct boost_gregorian_date_duration_tag {};
        struct unsupported_type_tag {};

    public:
        //! Function object return type
        typedef void result_type;
        //! Character type
        typedef typename base_type::char_type char_type;
        //! String type
        typedef typename base_type::string_type string_type;

    public:
        //! Default constructor
        basic_time_duration_formatter() :
            base_type(base_type::default_time_duration_format().c_str())
        {
        }
        //! Constructor with format setup
        template< typename ArgsT >
        explicit basic_time_duration_formatter(ArgsT const& args) :
            base_type(string_type(args[keywords::format | base_type::default_time_duration_format().str()]))
        {
        }
        //! Copy constructor
        basic_time_duration_formatter(basic_time_duration_formatter const& that) :
            base_type(static_cast< base_type const& >(that))
        {
        }

        //! Formatting method for result of difftime
        void operator()(double value)
        {
            if (value < (std::numeric_limits< long >::max)())
            {
                (*this)(posix_time::seconds(static_cast< long >(value)));
            }
            else
            {
                const long hrs = static_cast< long >(value / 3600.0);
                (*this)(posix_time::hours(hrs)
                    + posix_time::seconds(static_cast< long >(std::fmod(value, 3600.0))));
            }
        }
        //! Formatting method for other time duration objects
        template< typename T >
        void operator()(T const& value)
        {
            to_stream_dispatch(value, tag_of(boost::addressof(value)));
            this->m_Stream.flush();
        }

    private:
        template< typename T >
        void to_stream_dispatch(T const& value, boost_posix_time_duration_tag const&)
        {
            typedef boost::date_time::time_facet< posix_time::ptime, char_type > facet_t;
            this->BOOST_NESTED_TEMPLATE to_stream< facet_t >(value);
        }
        template< typename T >
        void to_stream_dispatch(T const& value, boost_gregorian_date_duration_tag const&)
        {
            typedef boost::date_time::date_facet< gregorian::date, char_type > facet_t;
            this->BOOST_NESTED_TEMPLATE to_stream< facet_t >(value);
        }
        template< typename T >
        void to_stream_dispatch(T const& value, unsupported_type_tag const&)
        {
            // Call a user-defined function to format the unknown type of date or time
            boost_log_format_time_duration(
                this->m_Stream, value, static_cast< string_type const& >(this->m_Format));
        }

        static boost_gregorian_date_duration_tag tag_of(gregorian::date_duration const*)
        {
            // We have to hardcode the Gregorian date duration type since
            // the time facet requires the date type to be instantiated.
            // And this type we cannot know from the date duration type.
            // I guess, I'll have to write date duration formatting by hand
            // one day to bypass the Boost.DateTime limitation...
            return boost_gregorian_date_duration_tag();
        }
        static boost_posix_time_duration_tag tag_of(posix_time::time_duration const*)
        {
            // We have to hardcode the POSIX time duration type since
            // the time facet requires the time type to be instantiated.
            // And this type we cannot know from the time duration type.
            // I guess, I'll have to write time duration formatting by hand
            // one day to bypass the Boost.DateTime limitation...
            return boost_posix_time_duration_tag();
        }
        static unsupported_type_tag tag_of(...)
        {
            return unsupported_type_tag();
        }
    };

    //! Time period formatting object
    template< typename CharT >
    class basic_time_period_formatter :
        public basic_date_time_formatter< CharT >
    {
        //! Base type
        typedef basic_date_time_formatter< CharT > base_type;

        //  A number of internal tags to detect the proper facet to use
        struct boost_time_period_tag {};
        struct unsupported_type_tag {};

    public:
        //! Character type
        typedef typename base_type::char_type char_type;
        //! String type
        typedef typename base_type::string_type string_type;

    private:
        //! Time period format
        const string_type m_PeriodFormat;
        //  The flags describe the format contents
        bool m_fHasBegin;
        bool m_fHasEnd;
        bool m_fHasLast;

    public:
        //! Default constructor
        basic_time_period_formatter() : m_PeriodFormat(base_type::default_time_period_format().str())
        {
            init_flags();
        }
        //! Constructor with format setup
        template< typename ArgsT >
        explicit basic_time_period_formatter(ArgsT const& args) :
            base_type(keywords::format = args[keywords::unit_format | base_type::default_date_time_format().str()]),
            m_PeriodFormat(args[keywords::format | base_type::default_time_period_format().str()])
        {
            init_flags();
        }
        //! Copy constructor
        basic_time_period_formatter(basic_time_period_formatter const& that) :
            base_type(static_cast< base_type const& >(that)),
            m_PeriodFormat(that.m_PeriodFormat),
            m_fHasBegin(that.m_fHasBegin),
            m_fHasEnd(that.m_fHasEnd),
            m_fHasLast(that.m_fHasLast)
        {
        }

        //! Formatting method for Boost.DateTime date object
        template< typename T >
        void operator()(T const& value)
        {
            to_stream_dispatch(value, tag_of(boost::addressof(value)));
            this->m_Stream.flush();
        }

    private:
        //! The function initializes the format description flags
        void init_flags()
        {
            typedef iterator_range< typename string_type::const_iterator > range_t;
            range_t range = boost::algorithm::find_first(
                m_PeriodFormat, base_type::time_period_begin_placeholder());
            m_fHasBegin = !range.empty();

            range = boost::algorithm::find_first(
                m_PeriodFormat, base_type::time_period_end_placeholder());
            m_fHasEnd = !range.empty();

            range = boost::algorithm::find_first(
                m_PeriodFormat, base_type::time_period_last_placeholder());
            m_fHasLast = !range.empty();
        }

        template< typename T >
        void to_stream_dispatch(T const& value, boost_time_period_tag const&)
        {
            string_type result = m_PeriodFormat;
            if (m_fHasBegin)
            {
                base_type::operator()(value.begin());
                boost::algorithm::replace_all(
                    result,
                    base_type::time_period_begin_placeholder(),
                    base_type::get());
                base_type::clear();
            }
            if (m_fHasEnd)
            {
                base_type::operator()(value.end());
                boost::algorithm::replace_all(
                    result,
                    base_type::time_period_end_placeholder(),
                    base_type::get());
                base_type::clear();
            }
            if (m_fHasLast)
            {
                base_type::operator()(value.last());
                boost::algorithm::replace_all(
                    result,
                    base_type::time_period_last_placeholder(),
                    base_type::get());
                base_type::clear();
            }
            base_type::m_Buffer.swap(result);
        }
        template< typename T >
        void to_stream_dispatch(T const& value, unsupported_type_tag const&)
        {
            // Call a user-defined function to format the unknown type of time period
            boost_log_format_time_period(
                this->m_Stream, value, static_cast< string_type const& >(this->m_Format), this->m_PeriodFormat);
        }

        template< typename TimePointT, typename DurationT >
        static boost_time_period_tag tag_of(boost::date_time::period< TimePointT, DurationT > const*)
        {
            return boost_time_period_tag();
        }
        static unsupported_type_tag tag_of(...)
        {
            return unsupported_type_tag();
        }
    };

} // namespace aux

/*!
 * \brief Date and time attribute formatter
 *
 * The formatter performs formatting for all date and time related types.
 */
template<
    typename CharT,
    typename AttributeValueTypesT,
    template< typename > class FormatterImplT,
    typename ExceptionPolicyT
>
class fmt_date_time_facade :
    public basic_formatter<
        CharT,
        fmt_date_time_facade< CharT, AttributeValueTypesT, FormatterImplT, ExceptionPolicyT >
    >
{
    //! Base type
    typedef basic_formatter<
        CharT,
        fmt_date_time_facade< CharT, AttributeValueTypesT, FormatterImplT, ExceptionPolicyT >
    > base_type;

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
    //! Actual formatter type
    typedef FormatterImplT< char_type > formatter_type;

private:
    //! Attribute value extractor
    attribute_value_extractor< char_type, AttributeValueTypesT > m_Extractor;
    //! Pointer to the formatter implementation
    mutable formatter_type m_Formatter;

public:
    /*!
     * Constructor
     *
     * \param name Attribute name
     */
    explicit fmt_date_time_facade(string_type const& name)
        : m_Extractor(name)
    {
    }
    /*!
     * Constructor with date and time format specification
     *
     * \param name Attribute name
     * \param args A set of named arguments that describe the format
     */
    template< typename ArgsT >
    fmt_date_time_facade(string_type const& name, ArgsT const& args)
        : m_Extractor(name), m_Formatter(args)
    {
    }

    /*!
     * Formatting operator. Formats the attribute with the specified on construction name from
     * \a attrs and puts the result into the \a strm stream.
     *
     * \param strm A reference to the stream, where the final text of the logging record is composed
     * \param record A logging record
     */
    void operator() (ostream_type& strm, record_type const& record) const
    {
        boost::log::aux::cleanup_guard< formatter_type > _(m_Formatter);
        if (!m_Extractor(record.attribute_values(), m_Formatter))
            ExceptionPolicyT::on_attribute_value_not_found(__FILE__, __LINE__);
        strm.write(m_Formatter.get().data(), static_cast< std::streamsize >(m_Formatter.get().size()));
    }
};


#ifndef BOOST_LOG_DOXYGEN_PASS

#ifdef BOOST_LOG_USE_CHAR

#   define BOOST_LOG_ITERATION_CHAR_TYPE char
#   define BOOST_LOG_ITERATION_KEYWORDS (format)

#   define BOOST_LOG_ITERATION_NAME date_time

#   define BOOST_PP_FILENAME_1 <boost/log/formatters/date_time.hpp>
#   define BOOST_PP_ITERATION_LIMITS (0, BOOST_PP_SEQ_SIZE(BOOST_LOG_ITERATION_KEYWORDS))
#   include BOOST_PP_ITERATE()

#   undef BOOST_LOG_ITERATION_NAME
#   define BOOST_LOG_ITERATION_NAME date

#   define BOOST_PP_FILENAME_1 <boost/log/formatters/date_time.hpp>
#   define BOOST_PP_ITERATION_LIMITS (0, BOOST_PP_SEQ_SIZE(BOOST_LOG_ITERATION_KEYWORDS))
#   include BOOST_PP_ITERATE()

#   undef BOOST_LOG_ITERATION_NAME
#   define BOOST_LOG_ITERATION_NAME time

#   define BOOST_PP_FILENAME_1 <boost/log/formatters/date_time.hpp>
#   define BOOST_PP_ITERATION_LIMITS (0, BOOST_PP_SEQ_SIZE(BOOST_LOG_ITERATION_KEYWORDS))
#   include BOOST_PP_ITERATE()

#   undef BOOST_LOG_ITERATION_NAME
#   define BOOST_LOG_ITERATION_NAME time_duration

#   define BOOST_PP_FILENAME_1 <boost/log/formatters/date_time.hpp>
#   define BOOST_PP_ITERATION_LIMITS (0, BOOST_PP_SEQ_SIZE(BOOST_LOG_ITERATION_KEYWORDS))
#   include BOOST_PP_ITERATE()

#   undef BOOST_LOG_ITERATION_NAME
#   define BOOST_LOG_ITERATION_NAME time_period
#   undef BOOST_LOG_ITERATION_KEYWORDS
#   define BOOST_LOG_ITERATION_KEYWORDS (format)(unit_format)

#   define BOOST_PP_FILENAME_1 <boost/log/formatters/date_time.hpp>
#   define BOOST_PP_ITERATION_LIMITS (0, BOOST_PP_SEQ_SIZE(BOOST_LOG_ITERATION_KEYWORDS))
#   include BOOST_PP_ITERATE()

#   undef BOOST_LOG_ITERATION_NAME
#   undef BOOST_LOG_ITERATION_CHAR_TYPE
#   undef BOOST_LOG_ITERATION_KEYWORDS

#endif // BOOST_LOG_USE_CHAR

#ifdef BOOST_LOG_USE_WCHAR_T

#   define BOOST_LOG_ITERATION_CHAR_TYPE wchar_t
#   define BOOST_LOG_ITERATION_KEYWORDS (format)

#   define BOOST_LOG_ITERATION_NAME date_time

#   define BOOST_PP_FILENAME_1 <boost/log/formatters/date_time.hpp>
#   define BOOST_PP_ITERATION_LIMITS (0, BOOST_PP_SEQ_SIZE(BOOST_LOG_ITERATION_KEYWORDS))
#   include BOOST_PP_ITERATE()

#   undef BOOST_LOG_ITERATION_NAME
#   define BOOST_LOG_ITERATION_NAME date

#   define BOOST_PP_FILENAME_1 <boost/log/formatters/date_time.hpp>
#   define BOOST_PP_ITERATION_LIMITS (0, BOOST_PP_SEQ_SIZE(BOOST_LOG_ITERATION_KEYWORDS))
#   include BOOST_PP_ITERATE()

#   undef BOOST_LOG_ITERATION_NAME
#   define BOOST_LOG_ITERATION_NAME time

#   define BOOST_PP_FILENAME_1 <boost/log/formatters/date_time.hpp>
#   define BOOST_PP_ITERATION_LIMITS (0, BOOST_PP_SEQ_SIZE(BOOST_LOG_ITERATION_KEYWORDS))
#   include BOOST_PP_ITERATE()

#   undef BOOST_LOG_ITERATION_NAME
#   define BOOST_LOG_ITERATION_NAME time_duration

#   define BOOST_PP_FILENAME_1 <boost/log/formatters/date_time.hpp>
#   define BOOST_PP_ITERATION_LIMITS (0, BOOST_PP_SEQ_SIZE(BOOST_LOG_ITERATION_KEYWORDS))
#   include BOOST_PP_ITERATE()

#   undef BOOST_LOG_ITERATION_NAME
#   define BOOST_LOG_ITERATION_NAME time_period
#   undef BOOST_LOG_ITERATION_KEYWORDS
#   define BOOST_LOG_ITERATION_KEYWORDS (format)(unit_format)

#   define BOOST_PP_FILENAME_1 <boost/log/formatters/date_time.hpp>
#   define BOOST_PP_ITERATION_LIMITS (0, BOOST_PP_SEQ_SIZE(BOOST_LOG_ITERATION_KEYWORDS))
#   include BOOST_PP_ITERATE()

#   undef BOOST_LOG_ITERATION_NAME
#   undef BOOST_LOG_ITERATION_CHAR_TYPE
#   undef BOOST_LOG_ITERATION_KEYWORDS

#endif // BOOST_LOG_USE_WCHAR_T

#else // BOOST_LOG_DOXYGEN_PASS

/*!
 * Formatter generator with the explicitly specified set of supported types in the first template parameter.
 *
 * \param name Attribute name
 * \param args Optional named arguments. Supported arguments:
 *             \li \c format - format string, must be compliant with Boost.DateTime date format string specification. Default: <tt>%%Y-%%b-%%d</tt>
 */
template< typename AttributeValueTypesT, typename CharT, typename... ArgsT >
fmt_date_time_facade<
    CharT,
    AttributeValueTypesT,
    implementation_defined,
    throw_policy
> date(std::basic_string< CharT > const& name, ArgsT... const& args);

/*!
 * Formatter generator with the explicitly specified set of supported types in the first template parameter.
 *
 * \param name Attribute name
 * \param args Optional named arguments. Supported arguments:
 *             \li \c format - format string, must be compliant with Boost.DateTime time format string specification. Default: <tt>%%H:%%M:%%S.%%f</tt>
 */
template< typename AttributeValueTypesT, typename CharT, typename... ArgsT >
fmt_date_time_facade<
    CharT,
    AttributeValueTypesT,
    implementation_defined,
    throw_policy
> time(std::basic_string< CharT > const& name, ArgsT... const& args);

/*!
 * Formatter generator with the explicitly specified set of supported types in the first template parameter.
 *
 * \param name Attribute name
 * \param args Optional named arguments. Supported arguments:
 *             \li \c format - format string, must be compliant with Boost.DateTime date and time format string specification. Default: <tt>%%Y-%%b-%%d %%H:%%M:%%S.%%f</tt>
 */
template< typename AttributeValueTypesT, typename CharT, typename... ArgsT >
fmt_date_time_facade<
    CharT,
    AttributeValueTypesT,
    implementation_defined,
    throw_policy
> date_time(std::basic_string< CharT > const& name, ArgsT... const& args);

/*!
 * Formatter generator with the explicitly specified set of supported types in the first template parameter.
 *
 * \param name Attribute name
 * \param args Optional named arguments. Supported arguments:
 *             \li \c format - format string, must be compliant with Boost.DateTime time format string specification. Default: <tt>%%-%%H:%%M:%%S.%%f</tt>
 */
template< typename AttributeValueTypesT, typename CharT, typename... ArgsT >
fmt_date_time_facade<
    CharT,
    AttributeValueTypesT,
    implementation_defined,
    throw_policy
> time_duration(std::basic_string< CharT > const& name, ArgsT... const& args);

/*!
 * Formatter generator with the explicitly specified set of supported types in the first template parameter.
 *
 * \param name Attribute name
 * \param args Optional named arguments. Supported arguments:
 *             \li \c unit_format - format string for period boundaries, must be compliant with Boost.DateTime date and time format string specification. Default: <tt>%%Y-%%b-%%d %%H:%%M:%%S.%%f</tt>
 *             \li \c format - format string for boundaries composition, can contain placeholders <tt>%%begin%</tt>, <tt>%%end%</tt> and <tt>%%last%</tt>. Default: <tt>[%%begin% - %%last%]</tt>
 */
template< typename AttributeValueTypesT, typename CharT, typename... ArgsT >
fmt_date_time_facade<
    CharT,
    AttributeValueTypesT,
    implementation_defined,
    throw_policy
> time_period(std::basic_string< CharT > const& name, ArgsT... const& args);


/*!
 * Formatter generator with the explicitly specified set of supported types in the first template parameter.
 * The formatter will not throw if the attribute value is not found in the record being formatted.
 * Instead, no output will be produced for this attribute value.
 *
 * \param name Attribute name
 * \param args Optional named arguments. Supported arguments:
 *             \li \c format - format string, must be compliant with Boost.DateTime date format string specification. Default: <tt>%%Y-%%b-%%d</tt>
 */
template< typename AttributeValueTypesT, typename CharT, typename... ArgsT >
fmt_date_time_facade<
    CharT,
    AttributeValueTypesT,
    implementation_defined,
    no_throw_policy
    > date(std::basic_string< CharT > const& name, ArgsT... const& args, std::nothrow_t const&);

/*!
 * Formatter generator with the explicitly specified set of supported types in the first template parameter.
 * The formatter will not throw if the attribute value is not found in the record being formatted.
 * Instead, no output will be produced for this attribute value.
 *
 * \param name Attribute name
 * \param args Optional named arguments. Supported arguments:
 *             \li \c format - format string, must be compliant with Boost.DateTime time format string specification. Default: <tt>%%H:%%M:%%S.%%f</tt>
 */
template< typename AttributeValueTypesT, typename CharT, typename... ArgsT >
fmt_date_time_facade<
    CharT,
    AttributeValueTypesT,
    implementation_defined,
    no_throw_policy
> time(std::basic_string< CharT > const& name, ArgsT... const& args, std::nothrow_t const&);

/*!
 * Formatter generator with the explicitly specified set of supported types in the first template parameter.
 * The formatter will not throw if the attribute value is not found in the record being formatted.
 * Instead, no output will be produced for this attribute value.
 *
 * \param name Attribute name
 * \param args Optional named arguments. Supported arguments:
 *             \li \c format - format string, must be compliant with Boost.DateTime date and time format string specification. Default: <tt>%%Y-%%b-%%d %%H:%%M:%%S.%%f</tt>
 */
template< typename AttributeValueTypesT, typename CharT, typename... ArgsT >
fmt_date_time_facade<
    CharT,
    AttributeValueTypesT,
    implementation_defined,
    no_throw_policy
> date_time(std::basic_string< CharT > const& name, ArgsT... const& args, std::nothrow_t const&);

/*!
 * Formatter generator with the explicitly specified set of supported types in the first template parameter.
 * The formatter will not throw if the attribute value is not found in the record being formatted.
 * Instead, no output will be produced for this attribute value.
 *
 * \param name Attribute name
 * \param args Optional named arguments. Supported arguments:
 *             \li \c format - format string, must be compliant with Boost.DateTime time format string specification. Default: <tt>%%-%%H:%%M:%%S.%%f</tt>
 */
template< typename AttributeValueTypesT, typename CharT, typename... ArgsT >
fmt_date_time_facade<
    CharT,
    AttributeValueTypesT,
    implementation_defined,
    no_throw_policy
> time_duration(std::basic_string< CharT > const& name, ArgsT... const& args, std::nothrow_t const&);

/*!
 * Formatter generator with the explicitly specified set of supported types in the first template parameter.
 * The formatter will not throw if the attribute value is not found in the record being formatted.
 * Instead, no output will be produced for this attribute value.
 *
 * \param name Attribute name
 * \param args Optional named arguments. Supported arguments:
 *             \li \c unit_format - format string for period boundaries, must be compliant with Boost.DateTime date and time format string specification. Default: <tt>%%Y-%%b-%%d %%H:%%M:%%S.%%f</tt>
 *             \li \c format - format string for boundaries composition, can contain placeholders <tt>%%begin%</tt>, <tt>%%end%</tt> and <tt>%%last%</tt>. Default: <tt>[%%begin% - %%last%]</tt>
 */
template< typename AttributeValueTypesT, typename CharT, typename... ArgsT >
fmt_date_time_facade<
    CharT,
    AttributeValueTypesT,
    implementation_defined,
    no_throw_policy
> time_period(std::basic_string< CharT > const& name, ArgsT... const& args, std::nothrow_t const&);

#endif // BOOST_LOG_DOXYGEN_PASS

} // namespace formatters

} // namespace log

} // namespace boost

#endif // BOOST_LOG_FORMATTERS_DATE_TIME_HPP_INCLUDED_

#ifdef BOOST_PP_IS_ITERATING

#define BOOST_LOG_ITERATION_DEFAULT_ATTR_TYPES\
    BOOST_PP_CAT(BOOST_LOG_ITERATION_NAME, _types)

#define BOOST_LOG_ITERATION_FORMATTER_IMPL\
    BOOST_PP_CAT(BOOST_PP_CAT(aux::basic_, BOOST_LOG_ITERATION_NAME), _formatter)

#define BOOST_LOG_ITERATION_MAKE_KEYWORD_TYPES(r, data, i, elem)\
    BOOST_PP_COMMA_IF(i) boost::parameter::optional<\
        keywords::tag::elem,\
        is_convertible< mpl::_, std::basic_string< BOOST_LOG_ITERATION_CHAR_TYPE > >\
    >

//! Formatter generator with the default list of supported attribute types
#if BOOST_PP_ITERATION() > 0
template< BOOST_PP_ENUM_PARAMS(BOOST_PP_ITERATION(), typename ArgT) >
#endif
inline fmt_date_time_facade<
    BOOST_LOG_ITERATION_CHAR_TYPE,
    BOOST_LOG_ITERATION_DEFAULT_ATTR_TYPES,
    BOOST_LOG_ITERATION_FORMATTER_IMPL,
    throw_policy
> BOOST_LOG_ITERATION_NAME(
    std::basic_string< BOOST_LOG_ITERATION_CHAR_TYPE > const& name BOOST_PP_COMMA_IF(BOOST_PP_ITERATION())
    BOOST_PP_ENUM_BINARY_PARAMS(BOOST_PP_ITERATION(), ArgT, const& arg)
)
{
    return fmt_date_time_facade<
        BOOST_LOG_ITERATION_CHAR_TYPE,
        BOOST_LOG_ITERATION_DEFAULT_ATTR_TYPES,
        BOOST_LOG_ITERATION_FORMATTER_IMPL,
        throw_policy
    >(
        name
#if BOOST_PP_ITERATION() > 0
        , boost::parameter::parameters<
            BOOST_PP_SEQ_FOR_EACH_I(BOOST_LOG_ITERATION_MAKE_KEYWORD_TYPES, ~, BOOST_LOG_ITERATION_KEYWORDS)
        >()(BOOST_PP_ENUM_PARAMS(BOOST_PP_ITERATION(), arg))
#endif
    );
}

//! Formatter generator with user-specified list of supported attribute types
template<
    typename AttributeValueTypesT BOOST_PP_COMMA_IF(BOOST_PP_ITERATION())
    BOOST_PP_ENUM_PARAMS(BOOST_PP_ITERATION(), typename ArgT)
>
inline fmt_date_time_facade<
    BOOST_LOG_ITERATION_CHAR_TYPE,
    AttributeValueTypesT,
    BOOST_LOG_ITERATION_FORMATTER_IMPL,
    throw_policy
> BOOST_LOG_ITERATION_NAME(
    std::basic_string< BOOST_LOG_ITERATION_CHAR_TYPE > const& name BOOST_PP_COMMA_IF(BOOST_PP_ITERATION())
    BOOST_PP_ENUM_BINARY_PARAMS(BOOST_PP_ITERATION(), ArgT, const& arg)
)
{
    return fmt_date_time_facade<
        BOOST_LOG_ITERATION_CHAR_TYPE,
        AttributeValueTypesT,
        BOOST_LOG_ITERATION_FORMATTER_IMPL,
        throw_policy
    >(
        name
#if BOOST_PP_ITERATION() > 0
        , boost::parameter::parameters<
            BOOST_PP_SEQ_FOR_EACH_I(BOOST_LOG_ITERATION_MAKE_KEYWORD_TYPES, ~, BOOST_LOG_ITERATION_KEYWORDS)
        >()(BOOST_PP_ENUM_PARAMS(BOOST_PP_ITERATION(), arg))
#endif
    );
}

//! Formatter generator with the default list of supported attribute types
#if BOOST_PP_ITERATION() > 0
template< BOOST_PP_ENUM_PARAMS(BOOST_PP_ITERATION(), typename ArgT) >
#endif
inline fmt_date_time_facade<
    BOOST_LOG_ITERATION_CHAR_TYPE,
    BOOST_LOG_ITERATION_DEFAULT_ATTR_TYPES,
    BOOST_LOG_ITERATION_FORMATTER_IMPL,
    no_throw_policy
> BOOST_LOG_ITERATION_NAME(
    std::basic_string< BOOST_LOG_ITERATION_CHAR_TYPE > const& name BOOST_PP_COMMA_IF(BOOST_PP_ITERATION())
    BOOST_PP_ENUM_BINARY_PARAMS(BOOST_PP_ITERATION(), ArgT, const& arg),
    std::nothrow_t const&
)
{
    return fmt_date_time_facade<
        BOOST_LOG_ITERATION_CHAR_TYPE,
        BOOST_LOG_ITERATION_DEFAULT_ATTR_TYPES,
        BOOST_LOG_ITERATION_FORMATTER_IMPL,
        no_throw_policy
    >(
        name
#if BOOST_PP_ITERATION() > 0
        , boost::parameter::parameters<
            BOOST_PP_SEQ_FOR_EACH_I(BOOST_LOG_ITERATION_MAKE_KEYWORD_TYPES, ~, BOOST_LOG_ITERATION_KEYWORDS)
        >()(BOOST_PP_ENUM_PARAMS(BOOST_PP_ITERATION(), arg))
#endif
    );
}

//! Formatter generator with user-specified list of supported attribute types
template<
    typename AttributeValueTypesT BOOST_PP_COMMA_IF(BOOST_PP_ITERATION())
    BOOST_PP_ENUM_PARAMS(BOOST_PP_ITERATION(), typename ArgT)
>
inline fmt_date_time_facade<
    BOOST_LOG_ITERATION_CHAR_TYPE,
    AttributeValueTypesT,
    BOOST_LOG_ITERATION_FORMATTER_IMPL,
    no_throw_policy
> BOOST_LOG_ITERATION_NAME(
    std::basic_string< BOOST_LOG_ITERATION_CHAR_TYPE > const& name BOOST_PP_COMMA_IF(BOOST_PP_ITERATION())
    BOOST_PP_ENUM_BINARY_PARAMS(BOOST_PP_ITERATION(), ArgT, const& arg),
    std::nothrow_t const&
)
{
    return fmt_date_time_facade<
        BOOST_LOG_ITERATION_CHAR_TYPE,
        AttributeValueTypesT,
        BOOST_LOG_ITERATION_FORMATTER_IMPL,
        no_throw_policy
    >(
        name
#if BOOST_PP_ITERATION() > 0
        , boost::parameter::parameters<
            BOOST_PP_SEQ_FOR_EACH_I(BOOST_LOG_ITERATION_MAKE_KEYWORD_TYPES, ~, BOOST_LOG_ITERATION_KEYWORDS)
        >()(BOOST_PP_ENUM_PARAMS(BOOST_PP_ITERATION(), arg))
#endif
    );
}

#undef BOOST_LOG_ITERATION_MAKE_KEYWORD_TYPES
#undef BOOST_LOG_ITERATION_FORMATTER_IMPL
#undef BOOST_LOG_ITERATION_DEFAULT_ATTR_TYPES

#endif // BOOST_PP_IS_ITERATING
