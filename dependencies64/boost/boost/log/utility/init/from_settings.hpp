/*
 *          Copyright Andrey Semashev 2007 - 2010.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   from_settings.hpp
 * \author Andrey Semashev
 * \date   11.10.2009
 *
 * The header contains definition of facilities that allows to initialize the library from
 * settings.
 */

#if (defined(_MSC_VER) && _MSC_VER > 1000)
#pragma once
#endif // _MSC_VER > 1000

#ifndef BOOST_LOG_UTILITY_INIT_FROM_SETTINGS_HPP_INCLUDED_
#define BOOST_LOG_UTILITY_INIT_FROM_SETTINGS_HPP_INCLUDED_

#include <map>
#include <string>
#include <boost/any.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/mpl/if.hpp>
#include <boost/function/function1.hpp>
#include <boost/log/detail/setup_prologue.hpp>
#include <boost/log/detail/embedded_string_type.hpp>
#include <boost/log/sinks/sink.hpp>

#ifdef _MSC_VER
#pragma warning(push)
// 'm_A' : class 'A' needs to have dll-interface to be used by clients of class 'B'
#pragma warning(disable: 4251)
// non dll-interface class 'A' used as base for dll-interface class 'B'
#pragma warning(disable: 4275)
#endif // _MSC_VER

namespace boost {

namespace BOOST_LOG_NAMESPACE {

/*!
 * \brief The class represents settings container
 *
 * All settings are presented as a number of named parameters divided into named sections.
 * There is only one layer of sections (i.e. there are no subsections), and all parameters
 * reside in sections (i.e. no top-level parameters). The parameters may have any type
 * compatible with <tt>boost::any</tt>, which is used to store parameters. Individual
 * parameters may be queried via subscript operators, like this:
 *
 * <code>
 * any param = settings["Section1"]["Param1"]; // reads parameter "Param1" in section "Section1"
 *                                             // returns empty value if no such parameter exists
 * settings["Section2"]["Param2"] = 10; // sets the parameter "Param2" in section "Section2" to value 10 (of type int)
 * </code>
 *
 * There are also other methods to work with parameters.
 */
template< typename CharT >
class basic_settings
{
public:
    //! Character type
    typedef CharT char_type;
    //! String type
    typedef std::basic_string< char_type > string_type;
    //! The type of the map of parameters and their names
    typedef std::map< string_type, any > parameters_type;
    //! The type of the map of sections
    typedef std::map< string_type, parameters_type > sections_type;

private:
#if !defined(BOOST_LOG_DOXYGEN_PASS)

    //! A reference to a single parameter proxy object
    template< bool IsConstV >
    class param_ref
    {
    private:
        typedef typename mpl::if_c<
            IsConstV,
            sections_type const&,
            sections_type&
        >::type sections_ref_type;

    private:
        sections_ref_type m_Sections;
        string_type const& m_SectionName;
        string_type const& m_ParamName;

    public:
        explicit param_ref(sections_ref_type sections, string_type const& section_name, string_type const& param_name) :
            m_Sections(sections),
            m_SectionName(section_name),
            m_ParamName(param_name)
        {
        }

        template< typename T >
        any& operator= (T const& val)
        {
            // This will only compile for non-const references
            any& param = m_Sections[m_SectionName][m_ParamName];
            // Wrap C-strings into STL strings, if needed
            typedef typename boost::log::aux::make_embedded_string_type< T >::type value_t;
            param = value_t(val);
            return param;
        }

        template< bool V >
        any& operator= (param_ref< V > const& val)
        {
            // This will only compile for non-const references
            any& param = m_Sections[m_SectionName][m_ParamName];
            param = val.operator any();
            return param;
        }

        operator any () const
        {
            typename sections_type::const_iterator section = m_Sections.find(m_SectionName);
            if (section != m_Sections.end())
            {
                typename parameters_type::const_iterator param = section->second.find(m_ParamName);
                if (param != section->second.end())
                {
                    return param->second;
                }
            }

            return any();
        }
    };

    //! A reference to parameters section proxy object
#ifndef BOOST_LOG_NO_MEMBER_TEMPLATE_FRIENDS
    template< bool IsConstV >
    class section_ref;
    template< bool IsConstV >
    friend class section_ref;
#endif
    template< bool IsConstV >
    class section_ref
    {
    private:
        typedef typename mpl::if_c<
            IsConstV,
            sections_type const&,
            sections_type&
        >::type sections_ref_type;

    private:
        sections_ref_type m_Sections;
        string_type const& m_SectionName;

    public:
        explicit section_ref(sections_ref_type sections, string_type const& section_name) :
            m_Sections(sections),
            m_SectionName(section_name)
        {
        }

        param_ref< IsConstV > operator[] (string_type const& param_name) const
        {
            return param_ref< IsConstV >(m_Sections, m_SectionName, param_name);
        }
    };

#endif // !defined(BOOST_LOG_DOXYGEN_PASS)

private:
    //! Parameters
    sections_type m_Sections;

public:
    /*!
     * Checks if the container is empty (i.e. contains no sections and parameters).
     */
    bool empty() const { return m_Sections.empty(); }

    /*!
     * Accessor to a single parameter. This operator should be used in conjunction
     * with the subsequent subscript operator that designates the parameter name.
     *
     * \param section_name The name of the section in which the parameter resides
     * \return An unspecified reference type that can be used for parameter name specifying
     */
#if !defined(BOOST_LOG_DOXYGEN_PASS)
    section_ref< false >
#else
    implementation_defined
#endif
    operator[] (string_type const& section_name)
    {
        return section_ref< false >(m_Sections, section_name);
    }
    /*!
     * Accessor to a single parameter. This operator should be used in conjunction
     * with the subsequent subscript operator that designates the parameter name.
     *
     * \param section_name The name of the section in which the parameter resides
     * \return An unspecified reference type that can be used for parameter name specifying
     */
#if !defined(BOOST_LOG_DOXYGEN_PASS)
    section_ref< true >
#else
    implementation_defined
#endif
    operator[] (string_type const& section_name) const
    {
        return section_ref< true >(m_Sections, section_name);
    }

    /*!
     * Accessor for the map of sections
     */
    sections_type const& sections() const { return m_Sections; }
    /*!
     * Accessor for the map of sections
     */
    sections_type& sections() { return m_Sections; }

    /*!
     * Checks if the specified section is present in the container.
     *
     * \param section_name The name of the section
     */
    bool has_section(string_type const& section_name) const
    {
        return (m_Sections.find(section_name) != m_Sections.end());
    }
    /*!
     * Checks if the specified parameter is present in the container.
     *
     * \param section_name The name of the section in which the parameter resides
     * \param param_name The name of the parameter
     */
    bool has_parameter(string_type const& section_name, string_type const& param_name) const
    {
        typename sections_type::const_iterator section = m_Sections.find(section_name);
        if (section != m_Sections.end())
            return (section->second.find(param_name) != section->second.end());
        else
            return false;
    }
};

#ifdef BOOST_LOG_USE_CHAR
typedef basic_settings< char > settings;        //!< Convenience typedef for narrow-character logging
#endif
#ifdef BOOST_LOG_USE_WCHAR_T
typedef basic_settings< wchar_t > wsettings;    //!< Convenience typedef for wide-character logging
#endif

/*!
 * The function initializes the logging library from a settings container
 *
 * \param setts Library settings container
 *
 * \b Throws: An <tt>std::exception</tt>-based exception if the provided settings are not valid.
 */
template< typename CharT >
BOOST_LOG_SETUP_EXPORT void init_from_settings(basic_settings< CharT > const& setts);


/*!
 * \brief The function registers a factory for a custom sink
 *
 * The function registers a factory for a sink. The factory will be called to create sink
 * instance when the parser discovers the specified sink type in the settings file. The
 * factory must accept a map of parameters [parameter name -> parameter value] that it
 * may use to initialize the sink. The factory must return a non-NULL pointer to the
 * constructed sink instance.
 *
 * \param sink_name The custom sink name. Must point to a zero-terminated sequence of characters,
 *                  must not be NULL.
 * \param factory Custom sink factory function
 */
template< typename CharT >
BOOST_LOG_SETUP_EXPORT void register_sink_factory(
    const CharT* sink_name,
    function1<
        shared_ptr< sinks::sink< CharT > >,
        std::map< std::basic_string< CharT >, any > const&
    > const& factory);

/*!
 * \brief The function registers a factory for a custom sink
 *
 * The function registers a factory for a sink. The factory will be called to create sink
 * instance when the parser discovers the specified sink type in the settings file. The
 * factory must accept a map of parameters [parameter name -> parameter value] that it
 * may use to initialize the sink. The factory must return a non-NULL pointer to the
 * constructed sink instance.
 *
 * \param sink_name The custom sink name
 * \param factory Custom sink factory function
 */
template< typename CharT, typename TraitsT, typename AllocatorT >
inline void register_sink_factory(
    std::basic_string< CharT, TraitsT, AllocatorT > const& sink_name,
    function1<
        shared_ptr< sinks::sink< CharT > >,
        std::map< std::basic_string< CharT >, any > const&
    > const& factory)
{
    register_sink_factory(sink_name.c_str(), factory);
}

} // namespace log

} // namespace boost

#ifdef _MSC_VER
#pragma warning(pop)
#endif // _MSC_VER

#endif // BOOST_LOG_UTILITY_INIT_FROM_SETTINGS_HPP_INCLUDED_
