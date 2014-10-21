/*
 *          Copyright Andrey Semashev 2007 - 2010.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file
 * \author Andrey Semashev
 * \date   31.10.2009
 *
 * The header contains exception classes declarations.
 */

#if (defined(_MSC_VER) && _MSC_VER > 1000)
#pragma once
#endif // _MSC_VER > 1000

#ifndef BOOST_LOG_EXCEPTIONS_HPP_INCLUDED_
#define BOOST_LOG_EXCEPTIONS_HPP_INCLUDED_

#include <string>
#include <stdexcept>
#include <boost/preprocessor/seq/enum.hpp>
#include <boost/compatibility/cpp_c_headers/cstddef>
#include <boost/log/detail/prologue.hpp>

#ifdef _MSC_VER
#pragma warning(push)
 // non dll-interface class 'A' used as base for dll-interface class 'B'
#pragma warning(disable: 4275)
#endif // _MSC_VER

namespace boost {

namespace BOOST_LOG_NAMESPACE {

/*!
 * \brief Base class for runtime exceptions from the logging library
 *
 * Exceptions derived from this class indicate problema that may not directly
 * be caused by the user's code that interacts with the library, such as
 * errors caused by input data.
 */
class BOOST_LOG_EXPORT runtime_error :
    public std::runtime_error
{
protected:
    /*!
     * Initializing constructor. Creates an exception with the specified error message.
     */
    explicit runtime_error(std::string const& descr);
    /*!
     * Destructor
     */
    ~runtime_error() throw();
};

/*!
 * \brief Exception class that is used to indicate errors of missing values
 */
class BOOST_LOG_EXPORT missing_value :
    public runtime_error
{
public:
    /*!
     * Default constructor. Creates an exception with the default error message.
     */
    missing_value();
    /*!
     * Initializing constructor. Creates an exception with the specified error message.
     */
    explicit missing_value(std::string const& descr);
    /*!
     * Destructor
     */
    ~missing_value() throw();

#ifndef BOOST_LOG_DOXYGEN_PASS
    static BOOST_LOG_NORETURN void throw_(const char* file, std::size_t line);
    static BOOST_LOG_NORETURN void throw_(const char* file, std::size_t line, std::string const& descr);
#endif
};

/*!
 * \brief Exception class that is used to indicate errors of incorrect type of an object
 */
class BOOST_LOG_EXPORT invalid_type :
    public runtime_error
{
public:
    /*!
     * Default constructor. Creates an exception with the default error message.
     */
    invalid_type();
    /*!
     * Initializing constructor. Creates an exception with the specified error message.
     */
    explicit invalid_type(std::string const& descr);
    /*!
     * Destructor
     */
    ~invalid_type() throw();

#ifndef BOOST_LOG_DOXYGEN_PASS
    static BOOST_LOG_NORETURN void throw_(const char* file, std::size_t line);
    static BOOST_LOG_NORETURN void throw_(const char* file, std::size_t line, std::string const& descr);
#endif
};

/*!
 * \brief Exception class that is used to indicate errors of incorrect value of an object
 */
class BOOST_LOG_EXPORT invalid_value :
    public runtime_error
{
public:
    /*!
     * Default constructor. Creates an exception with the default error message.
     */
    invalid_value();
    /*!
     * Initializing constructor. Creates an exception with the specified error message.
     */
    explicit invalid_value(std::string const& descr);
    /*!
     * Destructor
     */
    ~invalid_value() throw();

#ifndef BOOST_LOG_DOXYGEN_PASS
    static BOOST_LOG_NORETURN void throw_(const char* file, std::size_t line);
    static BOOST_LOG_NORETURN void throw_(const char* file, std::size_t line, std::string const& descr);
#endif
};

/*!
 * \brief Exception class that is used to indicate parsing errors
 */
class BOOST_LOG_EXPORT parse_error :
    public runtime_error
{
public:
    /*!
     * Default constructor. Creates an exception with the default error message.
     */
    parse_error();
    /*!
     * Initializing constructor. Creates an exception with the specified error message.
     */
    explicit parse_error(std::string const& descr);
    /*!
     * Destructor
     */
    ~parse_error() throw();

#ifndef BOOST_LOG_DOXYGEN_PASS
    static BOOST_LOG_NORETURN void throw_(const char* file, std::size_t line);
    static BOOST_LOG_NORETURN void throw_(const char* file, std::size_t line, std::string const& descr);
    static BOOST_LOG_NORETURN void throw_(const char* file, std::size_t line, std::string const& descr, std::size_t content_line);
#endif
};

/*!
 * \brief Exception class that is used to indicate conversion errors
 */
class BOOST_LOG_EXPORT conversion_error :
    public runtime_error
{
public:
    /*!
     * Default constructor. Creates an exception with the default error message.
     */
    conversion_error();
    /*!
     * Initializing constructor. Creates an exception with the specified error message.
     */
    explicit conversion_error(std::string const& descr);
    /*!
     * Destructor
     */
    ~conversion_error() throw();

#ifndef BOOST_LOG_DOXYGEN_PASS
    static BOOST_LOG_NORETURN void throw_(const char* file, std::size_t line);
    static BOOST_LOG_NORETURN void throw_(const char* file, std::size_t line, std::string const& descr);
#endif
};

/*!
 * \brief Exception class that is used to indicate underlying OS API errors
 */
class BOOST_LOG_EXPORT system_error :
    public runtime_error
{
public:
    /*!
     * Default constructor. Creates an exception with the default error message.
     */
    system_error();
    /*!
     * Initializing constructor. Creates an exception with the specified error message.
     */
    explicit system_error(std::string const& descr);
    /*!
     * Destructor
     */
    ~system_error() throw();

#ifndef BOOST_LOG_DOXYGEN_PASS
    static BOOST_LOG_NORETURN void throw_(const char* file, std::size_t line);
    static BOOST_LOG_NORETURN void throw_(const char* file, std::size_t line, std::string const& descr);
#endif
};

/*!
 * \brief Base class for logic exceptions from the logging library
 *
 * Exceptions derived from this class usually indicate errors on the user's side, such as
 * incorrect library usage.
 */
class BOOST_LOG_EXPORT logic_error :
    public std::logic_error
{
protected:
    /*!
     * Initializing constructor. Creates an exception with the specified error message.
     */
    explicit logic_error(std::string const& descr);
    /*!
     * Destructor
     */
    ~logic_error() throw();
};

/*!
 * \brief Exception class that is used to indicate ODR violation
 */
class BOOST_LOG_EXPORT odr_violation :
    public logic_error
{
public:
    /*!
     * Default constructor. Creates an exception with the default error message.
     */
    odr_violation();
    /*!
     * Initializing constructor. Creates an exception with the specified error message.
     */
    explicit odr_violation(std::string const& descr);
    /*!
     * Destructor
     */
    ~odr_violation() throw();

#ifndef BOOST_LOG_DOXYGEN_PASS
    static BOOST_LOG_NORETURN void throw_(const char* file, std::size_t line);
    static BOOST_LOG_NORETURN void throw_(const char* file, std::size_t line, std::string const& descr);
#endif
};

/*!
 * \brief Exception class that is used to indicate invalid call sequence
 */
class BOOST_LOG_EXPORT unexpected_call :
    public logic_error
{
public:
    /*!
     * Default constructor. Creates an exception with the default error message.
     */
    unexpected_call();
    /*!
     * Initializing constructor. Creates an exception with the specified error message.
     */
    explicit unexpected_call(std::string const& descr);
    /*!
     * Destructor
     */
    ~unexpected_call() throw();

#ifndef BOOST_LOG_DOXYGEN_PASS
    static BOOST_LOG_NORETURN void throw_(const char* file, std::size_t line);
    static BOOST_LOG_NORETURN void throw_(const char* file, std::size_t line, std::string const& descr);
#endif
};

/*!
 * \brief Exception class that is used to indicate invalid library setup
 */
class BOOST_LOG_EXPORT setup_error :
    public logic_error
{
public:
    /*!
     * Default constructor. Creates an exception with the default error message.
     */
    setup_error();
    /*!
     * Initializing constructor. Creates an exception with the specified error message.
     */
    explicit setup_error(std::string const& descr);
    /*!
     * Destructor
     */
    ~setup_error() throw();

#ifndef BOOST_LOG_DOXYGEN_PASS
    static BOOST_LOG_NORETURN void throw_(const char* file, std::size_t line);
    static BOOST_LOG_NORETURN void throw_(const char* file, std::size_t line, std::string const& descr);
#endif
};

} // namespace log

} // namespace boost

#ifndef BOOST_LOG_DOXYGEN_PASS

#define BOOST_LOG_THROW(ex)\
    ex::throw_(__FILE__, __LINE__)

#define BOOST_LOG_THROW_DESCR(ex, descr)\
    ex::throw_(__FILE__, __LINE__, descr)

#define BOOST_LOG_THROW_DESCR_PARAMS(ex, descr, params)\
    ex::throw_(__FILE__, __LINE__, descr, BOOST_PP_SEQ_ENUM(params))

#endif // BOOST_LOG_DOXYGEN_PASS

#ifdef _MSC_VER
#pragma warning(pop)
#endif // _MSC_VER

#endif // BOOST_LOG_EXCEPTIONS_HPP_INCLUDED_
