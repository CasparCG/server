/*
 *          Copyright Andrey Semashev 2007 - 2010.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   functor.hpp
 * \author Andrey Semashev
 * \date   24.06.2007
 *
 * The header contains implementation of an attribute that calls a third-party function on value acquirement.
 */

#if (defined(_MSC_VER) && _MSC_VER > 1000)
#pragma once
#endif // _MSC_VER > 1000

#ifndef BOOST_LOG_ATTRIBUTES_FUNCTOR_HPP_INCLUDED_
#define BOOST_LOG_ATTRIBUTES_FUNCTOR_HPP_INCLUDED_

#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>
#include <boost/static_assert.hpp>
#include <boost/utility/result_of.hpp>
#include <boost/type_traits/is_void.hpp>
#include <boost/type_traits/remove_cv.hpp>
#include <boost/type_traits/remove_reference.hpp>
#include <boost/log/detail/prologue.hpp>
#include <boost/log/attributes/attribute.hpp>
#include <boost/log/attributes/basic_attribute_value.hpp>

namespace boost {

namespace BOOST_LOG_NAMESPACE {

namespace attributes {

#ifndef BOOST_LOG_DOXYGEN_PASS

/*!
 * \brief A class of an attribute that acquires its value from a third-party functor
 *
 * The attribute calls a stored nullary functional object to acquire each value.
 * The result type of the functional object is the attribute value type.
 *
 * It is not recommended to use this class directly. Use \c make_functor_attr convenience functions
 * to construct the attribute instead.
 */
template< typename R, typename T >
class functor :
    public attribute
{
public:
    //! A held functor type
    typedef T held_type;

private:
    //! Attribute value type
    typedef basic_attribute_value< R > functor_result_value;

private:
    //! Functor that returns attribute values
    const held_type m_Functor;

public:
    /*!
     * Constructor with the stored delegate imitialization
     */
    explicit functor(held_type const& fun) : m_Functor(fun) {}

    shared_ptr< attribute_value > get_value()
    {
        return boost::make_shared< functor_result_value >(m_Functor());
    }
};

#endif // BOOST_LOG_DOXYGEN_PASS

#ifndef BOOST_NO_RESULT_OF

/*!
 * The function constructs functor attribute instance with the provided functional object.
 *
 * \param fun Nullary functional object that returns an actual stored value for an attribute value.
 * \return Pointer to the attribute instance
 */
template< typename T >
inline shared_ptr< attribute > make_functor_attr(T const& fun)
{
    typedef typename remove_cv<
        typename remove_reference<
            typename result_of< T() >::type
        >::type
    >::type result_type;
    BOOST_STATIC_ASSERT(!is_void< result_type >::value);

    typedef functor< result_type, T > functor_t;
    return boost::make_shared< functor_t >(fun);
}

#endif // BOOST_NO_RESULT_OF

#ifndef BOOST_LOG_DOXYGEN_PASS

/*!
 * The function constructs functor attribute instance with the provided functional object.
 * Use this version if your compiler fails to determine the result type of the functional object.
 *
 * \param fun Nullary functional object that returns an actual stored value for an attribute value.
 * \return Pointer to the attribute instance
 */
template< typename R, typename T >
inline shared_ptr< attribute > make_functor_attr(T const& fun)
{
    typedef typename remove_cv<
        typename remove_reference< R >::type
    >::type result_type;
    BOOST_STATIC_ASSERT(!is_void< result_type >::value);

    typedef functor< result_type, T > functor_t;
    return boost::make_shared< functor_t >(fun);
}

#endif // BOOST_LOG_DOXYGEN_PASS

} // namespace attributes

} // namespace log

} // namespace boost

#endif // BOOST_LOG_ATTRIBUTES_FUNCTOR_HPP_INCLUDED_
