#ifndef _FINDWRAPPER_H__
#define _FINDWRAPPER_H__

#include <io.h>
#include <string>

namespace caspar {
namespace utils {

	class FindWrapper
	{
		FindWrapper(const FindWrapper&);
		FindWrapper& operator=(const FindWrapper&);
	public:
		FindWrapper(const tstring&, WIN32_FIND_DATA*);
		~FindWrapper();

		bool FindNext(WIN32_FIND_DATA*);

		bool Success() {
			return (hFile_ != INVALID_HANDLE_VALUE);
		}

	private:
		HANDLE	hFile_;
	};

}	//namespace utils
}	//namespace caspar

#endif