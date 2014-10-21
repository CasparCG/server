/*
 *          Copyright Andrey Semashev 2007 - 2010.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   unspecified_bool.hpp
 * \author Andrey Semashev
 * \date   08.03.2009
 *
 * This header is the Boost.Log library implementation, see the library documentation
 * at http://www.boost.org/libs/log/doc/log.html.
 */

#if (defined(_MSC_VER) && _MSC_VER > 1000)
#pragma once
#endif // _MSC_VER > 1000

#ifndef BOOST_LOG_DETAIL_UNSPECIFIED_BOOL_HPP_INCLUDED_
#define BOOST_LOG_DETAIL_UNSPECIFIED_BOOL_HPP_INCLUDED_

#include <boost/log/detail/prologue.hpp>

#ifdef BOOST_LOG_DOXYGEN_PASS

#define BOOST_LOG_OPERATOR_UNSPECIFIED_BOOL()\
    public:\
        operator unspecified_bool () const;

#else

#ifndef BOOST_LOG_NO_UNSPECIFIED_BOOL

#define BOOST_LOG_OPERATOR_UNSPECIFIED_BOOL()\
    private:\
        struct dummy\
        {\
            int data1;\
            int data2;\
        };\
        typedef int (dummy::*unspecified_bool);\
    public:\
        operator unspecified_bool () const\
        {\
            if (!this->operator!())\
                return &dummy::data2;\
            else\
                return 0;\
        }

#else // BOOST_LOG_NO_UNSPECIFIED_BOOL

#define BOOST_LOG_OPERATOR_UNSPECIFIED_BOOL()\
    public:\
        operator bool () const\
        {\
            return !this->operator! ();\
        }

#endif // BOOST_LOG_NO_UNSPECIFIED_BOOL

#endif // BOOST_LOG_DOXYGEN_PASS

#endif // BOOST_LOG_DETAIL_UNSPECIFIED_BOOL_HPP_INCLUDED_
