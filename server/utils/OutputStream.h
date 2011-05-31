#ifndef OUTPUTSTREAM_H
#define OUTPUTSTREAM_H

namespace caspar {
namespace utils {

class OutputStream
{
	public:
		virtual ~OutputStream() {}

		virtual bool Open() = 0;
		virtual void Close() = 0;
		virtual void SetTimestamp(bool timestamp) = 0;
		virtual void Write(const void*, size_t) = 0;
		virtual void Print(const tstring& message) = 0;
		virtual void Println(const tstring& message) = 0;
};

typedef std::tr1::shared_ptr<OutputStream> OutputStreamPtr;

}
}

#endif