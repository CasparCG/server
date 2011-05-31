#ifndef FILEOUTPUTSTREAM_H
#define FILEOUTPUTSTREAM_H

#include "OutputStream.h"

#include <string>
#include <fstream>

namespace caspar {
namespace utils {

class FileOutputStream : public OutputStream, private LockableObject
{
	public:
		FileOutputStream(const TCHAR* filename, bool bAppend = false);
		virtual ~FileOutputStream();

		virtual bool Open();
		virtual void Close();
		virtual void SetTimestamp(bool timestamp);
		virtual void Print(const tstring& message);
		virtual void Println(const tstring& message);
		virtual void Write(const void* buffer, size_t);

	private:
		void WriteTimestamp();

	private:
		bool append;
		bool timestamp;
		tstring filename;
	
		#ifndef _UNICODE
			std::ofstream	outStream;
		#else
			std::wofstream	outStream;
		#endif
};

}
}

#endif