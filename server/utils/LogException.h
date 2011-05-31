#ifndef LOGEXCEPTION_H
#define LOGEXCEPTION_H

namespace caspar {
namespace utils {

class LogException : public std::runtime_error
{
	public:
		explicit LogException(const char* message = "Caught exception while logging");
		~LogException();

		const char* GetMassage() const;
};

}
}

#endif
