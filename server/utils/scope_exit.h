#ifndef _CASPAR_SCOPE_EXIT_H_
#define _CASPAR_SCOPE_EXIT_H_

#include <utility>
#include <functional>
#include "Noncopyable.hpp"

namespace caspar
{
	namespace utils 
	{
		class scope_exit
		{
			scope_exit( const scope_exit& );
			const scope_exit& operator=( const scope_exit& );
		public:
			
			template <typename T, typename F>
			explicit scope_exit(T& obj, const F& func) : exitScope_(std::bind(func, obj))
			{}

			explicit scope_exit(std::function<void()>&& exitScope) : exitScope_(std::move(exitScope))
			{}

			~scope_exit()
			{
				exitScope_();
			}

		private:
			std::function<void()> exitScope_;
		};		
		
	}
}

#define _CASPAR_EXIT_SCOPE_LINENAME_CAT(name, line) name##line
#define _CASPAR_EXIT_SCOPE_LINENAME(name, line) _CASPAR_EXIT_SCOPE_LINENAME_CAT(name, line)
#define CASPAR_SCOPE_EXIT caspar::utils::scope_exit _CASPAR_EXIT_SCOPE_LINENAME(EXIT, __LINE__)

#endif