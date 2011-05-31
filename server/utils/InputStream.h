#ifndef CASPAR_INPUTSTREAM_H__
#define CASPAR_INPUTSTREAM_H__

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

namespace caspar {
namespace utils {

class InputStream
{
public:
	virtual ~InputStream() {}

	virtual bool Open() = 0;
	virtual unsigned int Read(unsigned char*, size_t) = 0;
	virtual void Close() = 0;
};
typedef std::tr1::shared_ptr<InputStream> InputStreamPtr;

template <typename CharType>
class TextInputStream
{
public:
	virtual ~TextInputStream() {}

	virtual bool Open() = 0;
	virtual bool Readln(std::basic_string<CharType>&) = 0;
	virtual unsigned int Read(CharType*, size_t) = 0;
	virtual void Close() = 0;
};

#ifdef UNICODE
	typedef std::tr1::shared_ptr<TextInputStream<wchar_t> > TextInputStreamPtr;
#else
	typedef std::tr1::shared_ptr<TextInputStream<char> > TextInputStreamPtr;
#endif

}	//namespace utils
}	//namespace caspar

#endif