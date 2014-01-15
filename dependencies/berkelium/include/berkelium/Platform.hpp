#ifndef _BERKELIUM_PLATFORM_HPP_
#define _BERKELIUM_PLATFORM_HPP_


#define PLATFORM_WINDOWS 0
#define PLATFORM_LINUX   1
#define PLATFORM_MAC     2


#if defined(__WIN32__) || defined(_WIN32)
// disable type needs to have dll-interface to be used byu clients due to STL member variables which are not public
#pragma warning (disable: 4251)
//disable warning about no suitable definition provided for explicit template instantiation request which seems to have no resolution nor cause any problems
#pragma warning (disable: 4661)
//disable non dll-interface class used as base for dll-interface class when deriving from singleton
#pragma warning (disable : 4275)
#  define BERKELIUM_PLATFORM PLATFORM_WINDOWS
#elif defined(__APPLE_CC__) || defined(__APPLE__)
#  define BERKELIUM_PLATFORM PLATFORM_MAC
#  ifndef __MACOSX__
#    define __MACOSX__
#  endif
#else
#  define BERKELIUM_PLATFORM PLATFORM_LINUX
#endif

#ifdef NDEBUG
#define BERKELIUM_DEBUG 0
#else
#define BERKELIUM_DEBUG 1
#endif

#ifndef BERKELIUM_EXPORT
# if BERKELIUM_PLATFORM == PLATFORM_WINDOWS
#   if defined(STATIC_LINKED)
#     define BERKELIUM_EXPORT
#   else
#     if defined(BERKELIUM_BUILD)
#       define BERKELIUM_EXPORT __declspec(dllexport)
#     else
#       define BERKELIUM_EXPORT __declspec(dllimport)
#     endif
#   endif
#   define BERKELIUM_PLUGIN_EXPORT __declspec(dllexport)
# else
#   if defined(__GNUC__) && __GNUC__ >= 4
#     define BERKELIUM_EXPORT __attribute__ ((visibility("default")))
#     define BERKELIUM_PLUGIN_EXPORT __attribute__ ((visibility("default")))
#   else
#     define BERKELIUM_EXPORT
#     define BERKELIUM_PLUGIN_EXPORT
#   endif
# endif
#endif


#ifndef BERKELIUM_FUNCTION_EXPORT
# if BERKELIUM_PLATFORM == PLATFORM_WINDOWS
#   if defined(STATIC_LINKED)
#     define BERKELIUM_FUNCTION_EXPORT
#   else
#     if defined(BERKELIUM_BUILD)
#       define BERKELIUM_FUNCTION_EXPORT __declspec(dllexport)
#     else
#       define BERKELIUM_FUNCTION_EXPORT __declspec(dllimport)
#     endif
#   endif
# else
#   define BERKELIUM_FUNCTION_EXPORT
# endif
#endif

#endif

#define UNIMPLEMENTED() (fprintf(stderr,"UNIMPLEMENTED %s:%d\n",__FILE__,__LINE__))


#include <cstddef>
