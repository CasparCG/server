#pragma once

#include <utility>
#include <functional>

#include <boost/noncopyable.hpp>

namespace caspar
{
	namespace detail 
	{
		template<typename T>
		class scope_exit : boost::noncopyable
		{
		public:			
			explicit scope_exit(T&& exitScope) : exitScope_(std::forward<T>(exitScope)){}
			~scope_exit()
			{
				try
				{
					exitScope_();
				}
				catch(...)
				{
					CASPAR_LOG_CURRENT_EXCEPTION();
				}
			}
		private:
			T exitScope_;
		};			

		template <typename T>
		scope_exit<T> create_scope_exit(T&& exitScope)
		{
			return scope_exit<T>(std::forward<T>(exitScope));
		}
	}
}

#define _CASPAR_EXIT_SCOPE_LINENAME_CAT(name, line) name##line
#define _CASPAR_EXIT_SCOPE_LINENAME(name, line) _CASPAR_EXIT_SCOPE_LINENAME_CAT(name, line)
#define CASPAR_SCOPE_EXIT(f) const auto& _CASPAR_EXIT_SCOPE_LINENAME(EXIT, __LINE__) = caspar::detail::create_scope_exit(f); _CASPAR_EXIT_SCOPE_LINENAME(EXIT, __LINE__)