#ifndef __NONCOPYABLE_H__
#define __NONCOPYABLE_H__

namespace caspar {
namespace utils {

class Noncopyable
{
protected:
	Noncopyable() {}
	~Noncopyable() {}
private:
	Noncopyable( const Noncopyable& );
	const Noncopyable& operator=( const Noncopyable& );
};

} // namespace utils
} //namespace caspar

#endif __NONCOPYABLE_H__