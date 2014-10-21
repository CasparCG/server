/*
 *          Copyright Andrey Semashev 2007 - 2010.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   text_multifile_backend.hpp
 * \author Andrey Semashev
 * \date   09.06.2009
 *
 * The header contains implementation of a text multi-file sink backend.
 */

#if (defined(_MSC_VER) && _MSC_VER > 1000)
#pragma once
#endif // _MSC_VER > 1000

#ifndef BOOST_LOG_SINKS_TEXT_MULTIFILE_BACKEND_HPP_INCLUDED_
#define BOOST_LOG_SINKS_TEXT_MULTIFILE_BACKEND_HPP_INCLUDED_

#include <ios>
#include <string>
#include <locale>
#include <ostream>
#include <boost/mpl/if.hpp>
#include <boost/mpl/bool.hpp>
#include <boost/type_traits/is_same.hpp>
#include <boost/function/function1.hpp>
#include <boost/log/detail/prologue.hpp>
#include <boost/log/detail/universal_path.hpp>
#include <boost/log/detail/cleanup_scope_guard.hpp>
#include <boost/log/detail/code_conversion.hpp>
#include <boost/log/detail/attachable_sstream_buf.hpp>
#include <boost/log/formatters/basic_formatters.hpp> // is_formatter
#include <boost/log/sinks/basic_sink_backend.hpp>

#ifdef _MSC_VER
#pragma warning(push)
// 'm_A' : class 'A' needs to have dll-interface to be used by clients of class 'B'
#pragma warning(disable: 4251)
// non dll-interface class 'A' used as base for dll-interface class 'B'
#pragma warning(disable: 4275)
#endif // _MSC_VER

namespace boost {

namespace BOOST_LOG_NAMESPACE {

namespace sinks {

namespace file {

    /*!
     * An adapter class that allows to use regular formatters as file name generators.
     */
    template< typename FormatterT >
    class file_name_composer_adapter
    {
    public:
        //! Functor result type
        typedef boost::log::aux::universal_path result_type;
        //! The adopted formatter type
        typedef FormatterT formatter_type;
        //! Character type used by the formatter
        typedef typename formatter_type::char_type char_type;
        //! String type used by the formatter
        typedef typename formatter_type::string_type string_type;
        //! Log record type
        typedef typename formatter_type::record_type record_type;

        //! Stream buffer type used to store formatted data
        typedef typename mpl::if_<
            is_same< char_type, result_type::string_type::value_type >,
            boost::log::aux::basic_ostringstreambuf< char_type >,
            boost::log::aux::converting_ostringstreambuf< char_type >
        >::type streambuf_type;
        //! Stream type used for formatting
        typedef std::basic_ostream< char_type > stream_type;

    private:
        //! The adopted formatter
        formatter_type m_Formatter;
        //! Formatted file name storage
        mutable result_type::string_type m_FileName;
        //! Stream buffer to fill the storage
        mutable streambuf_type m_StreamBuf;
        //! Formatting stream
        mutable stream_type m_FormattingStream;

    public:
        /*!
         * Initializing constructor
         */
        explicit file_name_composer_adapter(formatter_type const& formatter, std::locale const& loc = std::locale()) :
            m_Formatter(formatter),
            m_StreamBuf(m_FileName),
            m_FormattingStream(&m_StreamBuf)
        {
            m_FormattingStream.exceptions(std::ios_base::badbit | std::ios_base::failbit);
            m_FormattingStream.imbue(loc);
        }
        /*!
         * Copy constructor
         */
        file_name_composer_adapter(file_name_composer_adapter const& that) :
            m_Formatter(that.m_Formatter),
            m_StreamBuf(m_FileName),
            m_FormattingStream(&m_StreamBuf)
        {
            m_FormattingStream.exceptions(std::ios_base::badbit | std::ios_base::failbit);
            m_FormattingStream.imbue(that.m_FormattingStream.getloc());
        }
        /*!
         * Assignment
         */
        file_name_composer_adapter& operator= (file_name_composer_adapter const& that)
        {
            m_Formatter = that.m_Formatter;
            return *this;
        }

        /*!
         * The operator generates a file name based on the log record
         */
        result_type operator() (record_type const& record) const
        {
            boost::log::aux::cleanup_guard< stream_type > cleanup1(m_FormattingStream);
            boost::log::aux::cleanup_guard< streambuf_type > cleanup2(m_StreamBuf);
            boost::log::aux::cleanup_guard< result_type::string_type > cleanup3(m_FileName);

            m_Formatter(m_FormattingStream, record);
            m_FormattingStream.flush();

            return result_type(m_FileName);
        }
    };

    /*!
     * The function adopts a log record formatter into a file name generator
     */
    template< typename FormatterT >
    inline file_name_composer_adapter< FormatterT > as_file_name_composer(
        FormatterT const& fmt, std::locale const& loc = std::locale())
    {
        return file_name_composer_adapter< FormatterT >(fmt, loc);
    }

} // namespace file


/*!
 * \brief An implementation of a text multiple files logging sink backend
 *
 * The sink backend puts formatted log records to one of the text files.
 * The particular file is chosen upon each record's attribute values, which allows
 * to distribute records into individual files or to group records related to
 * some entity or process in a separate file.
 */
template< typename CharT >
class basic_text_multifile_backend :
    public basic_formatting_sink_backend< CharT >
{
    //! Base type
    typedef basic_formatting_sink_backend< CharT > base_type;

public:
    //! Character type
    typedef typename base_type::char_type char_type;
    //! String type to be used as a message text holder
    typedef typename base_type::string_type string_type;
    //! String type to be used as a message text holder
    typedef typename base_type::target_string_type target_string_type;
    //! Log record type
    typedef typename base_type::record_type record_type;
    //! Path type that is used by Boost.Log
    typedef boost::log::aux::universal_path path_type;

    //! File name composer functor type
    typedef function1< path_type, record_type const& > file_name_composer_type;

private:
    //! \cond

    struct implementation;
    implementation* m_pImpl;

    //! \endcond

public:
    /*!
     * Default constructor. The constructed sink backend has no file name composer and
     * thus will not write any files.
     */
    BOOST_LOG_EXPORT basic_text_multifile_backend();

    /*!
     * Destructor
     */
    BOOST_LOG_EXPORT ~basic_text_multifile_backend();

    /*!
     * The method sets file name composer functional object. Log record formatters are accepted, too.
     *
     * \param composer File name composer functor
     */
    template< typename ComposerT >
    void set_file_name_composer(ComposerT const& composer)
    {
        set_file_name_composer_internal(composer, typename formatters::is_formatter< ComposerT >::type());
    }

private:
#ifndef BOOST_LOG_DOXYGEN_PASS
    //! The method writes the message to the sink
    BOOST_LOG_EXPORT void do_consume(record_type const& record, target_string_type const& formatted_message);

    //! The method sets the file name composer
    template< typename ComposerT >
    void set_file_name_composer_internal(ComposerT const& composer, mpl::true_ const&)
    {
        set_file_name_composer_internal(file::as_file_name_composer(composer));
    }
    //! The method sets the file name composer
    template< typename ComposerT >
    void set_file_name_composer(ComposerT const& composer, mpl::false_ const&)
    {
        set_file_name_composer_internal(composer);
    }
    //! The method sets the file name composer
    BOOST_LOG_EXPORT void set_file_name_composer_internal(file_name_composer_type const& composer);
#endif // BOOST_LOG_DOXYGEN_PASS
};

#ifdef BOOST_LOG_USE_CHAR
typedef basic_text_multifile_backend< char > text_multifile_backend;     //!< Convenience typedef for narrow-character logging
#endif
#ifdef BOOST_LOG_USE_WCHAR_T
typedef basic_text_multifile_backend< wchar_t > wtext_multifile_backend; //!< Convenience typedef for wide-character logging
#endif

} // namespace sinks

} // namespace log

} // namespace boost

#ifdef _MSC_VER
#pragma warning(pop)
#endif // _MSC_VER

#endif // BOOST_LOG_SINKS_TEXT_MULTIFILE_BACKEND_HPP_INCLUDED_
