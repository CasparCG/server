#pragma once

namespace caspar {

template<typename def, typename inner = typename def::type>
class enum_class : public def
{
	typedef typename def::type type;
	inner val_; 
public: 
	explicit enum_class(int v) : val_(static_cast<type>(v)) {}
	enum_class(type v) : val_(v) {}
	inner value() const { return val_; }
 
	bool operator==(const enum_class& s) const { return val_ == s.val_; }
	bool operator!=(const enum_class& s) const { return val_ != s.val_; }
	bool operator<(const enum_class& s) const { return val_ <  s.val_; }
	bool operator<=(const enum_class& s) const { return val_ <= s.val_; }
	bool operator>(const enum_class& s) const { return val_ >  s.val_; }
	bool operator>=(const enum_class& s) const { return val_ >= s.val_; }

	enum_class operator&(const enum_class& s) const
	{
		return enum_class(static_cast<type>(val_ & s.val_));
	}

	enum_class operator|(const enum_class& s) const
	{
		return enum_class(static_cast<type>(val_ | s.val_));
	}

	//operator inner()
	//{
	//	return val_;
	//}
};

#define CASPAR_ENUM_CLASS_DEF(name) name ## _def
#define CASPAR_BEGIN_ENUM_CLASS typedef struct { enum type				
#define CASPAR_END_ENUM_CLASS(name) ;} CASPAR_ENUM_CLASS_DEF(name); typedef enum_class<CASPAR_ENUM_CLASS_DEF(name)> name;

}