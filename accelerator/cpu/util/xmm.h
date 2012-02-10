#pragma once

#include <intrin.h>

#include <type_traits>
#include <vector>
#include <tbb/cache_aligned_allocator.h>

namespace caspar { namespace accelerator { namespace cpu { namespace xmm {

typedef std::vector<float, tbb::cache_aligned_allocator<float>> vector_ps;

struct stream_tag
{
	static const int value = 0x01;
};
struct store_tag
{
	static const int value = 0x02;
};

class s32_x;
class s16_x;
class  s8_x;
class  u8_x;

template<typename T>
class base_x
{
public:
	static T load(const void* source);
	static T loadu(const void* source);
	static T zero();
	
	static void write(const T& source, void* dest, stream_tag);
	static void write(const T& source, void* dest, store_tag);
	static void stream(const T& source, void* dest);
	static void store(const T& source, void* dest);
};

class s32_x : public base_x<s32_x>
{
	__m128i value_;
	template<typename> friend class base_x;
	friend class s16_x;
	friend class s8_x;
	friend class u8_x;
public:
	typedef s32_x xmm_epi_tag;

	s32_x();
	s32_x(const s16_x& other);
	s32_x(const s8_x& other);
	s32_x(const u8_x& other);
	s32_x(const __m128i& value);

	s32_x& operator>>=(int count);
	s32_x& operator<<=(int count);
	s32_x& operator|=(const s32_x& other);
	s32_x& operator&=(const s32_x& other);
	int32_t operator[](int index) const;
	int32_t& operator[](int index);
};

class s16_x : public base_x<s16_x>
{
	__m128i value_;

private:
	template<typename> friend class base_x;
	friend class s32_x;
	friend class s8_x;
	friend class u8_x;
public:
	typedef s16_x xmm_epi_tag;

	s16_x();
	s16_x(const s32_x& other);
	s16_x(const s8_x& other);
	s16_x(const u8_x& other);
	s16_x(const __m128i& value);
	s16_x(short value);

	s16_x& operator+=(const s16_x& other);	
	s16_x& operator-=(const s16_x& other);
	s16_x& operator>>=(int count);
	s16_x& operator<<=(int count);
	s16_x& operator|=(const s16_x& other);
	s16_x& operator&=(const s16_x& other);	
	int16_t operator[](int index) const;
	int16_t& operator[](int index);
	
	static s16_x unpack_low(const s8_x& lhs, const s8_x& rhs);
	static s16_x unpack_high(const s8_x& lhs, const s8_x& rhs);
	static s32_x horizontal_add(const s16_x& lhs);
	static s16_x multiply_low(const s16_x& lhs, const s16_x& rhs);
	static s16_x multiply_high(const s16_x& lhs, const s16_x& rhs);
	static s16_x umultiply_low(const s16_x& lhs, const s16_x& rhs);
	static s16_x umultiply_high(const s16_x& lhs, const s16_x& rhs);	
	static s16_x unpack_low(const s16_x& lhs, const s16_x& rhs);
	static s16_x unpack_high(const s16_x& lhs, const s16_x& rhs);
	static s16_x and_not(const s16_x& lhs, const s16_x& rhs);	
	static s16_x max(const s16_x& lhs, const s16_x& rhs);
	static s16_x min(const s16_x& lhs, const s16_x& rhs);
};

class s8_x : public base_x<s8_x>
{
	__m128i value_;
private:
	template<typename> friend class base_x;
	friend class s32_x;
	friend class s16_x;
	friend class u8_x;
public:
	typedef s8_x xmm_epi_tag;

	s8_x();
	s8_x(const s32_x& other);
	s8_x(const s16_x& other);
	s8_x(const u8_x& other);
	s8_x(const __m128i& value);	
	s8_x(char b);
	s8_x(char b3,  char b2,  char b1,  char b0);
	s8_x(char b15, char b14, char b13, char b12, 
			 char b11, char b10, char b9,  char b8,  
			 char b7,  char b6,  char b5,  char b4,  
			 char b3,  char b2,  char b1,  char b0);

	s8_x& operator+=(const s8_x& other);
	s8_x& operator-=(const s8_x& other);									
	char operator[](int index) const;
	char& operator[](int index);
	
	static s8_x upack(const s16_x& lhs, const s16_x& rhs);

	static s16_x multiply_add(const s8_x& lhs, const s8_x& rhs);
	static s8_x shuffle(const s8_x& lhs, const s8_x& rhs);
	static s8_x max(const s8_x& lhs, const s8_x& rhs);
	static s8_x min(const s8_x& lhs, const s8_x& rhs);
	static s8_x blend(const s8_x& lhs, const s8_x& rhs, const s8_x& mask);
	static s8_x zero();
};

class u8_x : public base_x<u8_x>
{
	__m128i value_;
private:
	template<typename> friend class base_x;
	friend class s32_x;
	friend class s16_x;
	friend class s8_x;
public:
	typedef u8_x xmm_epu_tag;

	u8_x();
	u8_x(const s32_x& other);
	u8_x(const s16_x& other);
	u8_x(const s8_x& other);
	u8_x(const __m128i& value);	
	u8_x(char b);
	u8_x(char b3,  char b2,  char b1,  char b0);
	u8_x(char b15, char b14, char b13, char b12, 
			 char b11, char b10, char b9,  char b8,  
			 char b7,  char b6,  char b5,  char b4,  
			 char b3,  char b2,  char b1,  char b0);
										
	char operator[](int index) const;
	char& operator[](int index);
			
	static u8_x max(const u8_x& lhs, const u8_x& rhs);
	static u8_x min(const u8_x& lhs, const u8_x& rhs);
};

// base_x

template<typename T>
T base_x<T>::load(const void* source)
{
	return _mm_load_si128(reinterpret_cast<const __m128i*>(source));
}
	
template<typename T>
T base_x<T>::loadu(const void* source)
{
	return _mm_loadu_si128(reinterpret_cast<const __m128i*>(source));
}

template<typename T>
T base_x<T>::zero()
{
	return _mm_setzero_si128();
}

template<typename T>
void base_x<T>::write(const T& source, void* dest, store_tag)
{
	base_x<T>::store(source, dest);
}

template<typename T>
void base_x<T>::write(const T& source, void* dest, stream_tag)
{
	base_x<T>::stream(source, dest);
}

template<typename T>
void base_x<T>::stream(const T& source, void* dest)
{
	_mm_stream_si128(reinterpret_cast<__m128i*>(dest), source.value_);
}

template<typename T>
void base_x<T>::store(const T& source, void* dest)
{	
	_mm_store_si128(reinterpret_cast<__m128i*>(dest), source.value_);
}

// s32_x

s32_x::s32_x()
{
}

s32_x::s32_x(const s16_x& other)
	: value_(other.value_)
{
}

s32_x::s32_x(const s8_x& other)
	: value_(other.value_)
{
}

s32_x::s32_x(const u8_x& other)
	: value_(other.value_)
{
}

s32_x::s32_x(const __m128i& value)
	: value_(value)
{
}
	
s32_x& s32_x::operator>>=(int count)
{
	value_ = _mm_srli_epi32(value_, count);
	return *this;
}
	
s32_x& s32_x::operator<<=(int count)
{
	value_ = _mm_slli_epi32(value_, count);
	return *this;
}
		
s32_x& s32_x::operator|=(const s32_x& other)
{
	value_ = _mm_or_si128(value_, other.value_);
	return *this;
}	
	
s32_x& s32_x::operator&=(const s32_x& other)
{
	value_ = _mm_and_si128(value_, other.value_);
	return *this;
}	
		
int32_t s32_x::operator[](int index) const
{
	return value_.m128i_i32[index];
}

int32_t& s32_x::operator[](int index)
{
	return value_.m128i_i32[index];
}

inline s32_x operator>>(const s32_x& lhs, int count)
{		
	return s32_x(lhs) >>= count;
}

inline s32_x operator<<(const s32_x& lhs, int count)
{		
	return s32_x(lhs) <<= count;
}

inline s32_x operator|(const s32_x& lhs, const s32_x& rhs)
{		
	return s32_x(lhs) |= rhs;
}

inline s32_x operator&(const s32_x& lhs, const s32_x& rhs)
{		
	return s32_x(lhs) &= rhs;
}

// s16_x

s16_x::s16_x()
{
}

s16_x::s16_x(const s32_x& other)
	: value_(other.value_)
{
}

s16_x::s16_x(const s8_x& other)
	: value_(other.value_)
{
}

s16_x::s16_x(const u8_x& other)
	: value_(other.value_)
{
}

s16_x::s16_x(const __m128i& value)
	: value_(value)
{
}

s16_x::s16_x(short value)
	: value_(_mm_set1_epi16(value))
{
}

s16_x& s16_x::operator+=(const s16_x& other)
{
	value_ = _mm_add_epi16(value_, other.value_);
	return *this;
}
	
s16_x& s16_x::operator-=(const s16_x& other)
{
	value_ = _mm_sub_epi16(value_, other.value_);
	return *this;
}

s16_x& s16_x::operator>>=(int count)
{
	value_ = _mm_srli_epi16(value_, count);
	return *this;
}
	
s16_x& s16_x::operator<<=(int count)
{
	value_ = _mm_slli_epi16(value_, count);
	return *this;
}

s16_x& s16_x::operator|=(const s16_x& other)
{
	value_ = _mm_or_si128(value_, other.value_);
	return *this;
}	
	
s16_x& s16_x::operator&=(const s16_x& other)
{
	value_ = _mm_and_si128(value_, other.value_);
	return *this;
}	
			
int16_t s16_x::operator[](int index) const
{
	return value_.m128i_i16[index];
}

int16_t& s16_x::operator[](int index)
{
	return value_.m128i_i16[index];
}

s16_x s16_x::unpack_low(const s8_x& lhs, const s8_x& rhs)
{
	return _mm_unpacklo_epi8(rhs.value_, lhs.value_);
}
	
s16_x s16_x::unpack_high(const s8_x& lhs, const s8_x& rhs)
{
	return _mm_unpackhi_epi8(rhs.value_, lhs.value_);
}
	
s32_x s16_x::horizontal_add(const s16_x& lhs)
{
	#ifdef SSIM_XOP
			return _mm_haddd_epi16(value_);
	#else
			return _mm_madd_epi16(lhs.value_, _mm_set1_epi16(1));
	#endif
}

s16_x s16_x::multiply_low(const s16_x& lhs, const s16_x& rhs)
{
	return _mm_mullo_epi16(lhs.value_, rhs.value_);
}

s16_x s16_x::multiply_high(const s16_x& lhs, const s16_x& rhs)
{
	return _mm_mulhi_epi16(lhs.value_, rhs.value_);
}

s16_x s16_x::unpack_low(const s16_x& lhs, const s16_x& rhs)
{
	return _mm_unpacklo_epi16(lhs.value_, rhs.value_);
}

s16_x s16_x::unpack_high(const s16_x& lhs, const s16_x& rhs)
{
	return _mm_unpackhi_epi16(lhs.value_, rhs.value_);
}
	
s16_x s16_x::and_not(const s16_x& lhs, const s16_x& rhs)
{
	return _mm_andnot_si128(lhs.value_, rhs.value_);
}
	
s16_x s16_x::max(const s16_x& lhs, const s16_x& rhs)
{
	return _mm_max_epi16(lhs.value_, rhs.value_);
}
	
s16_x s16_x::min(const s16_x& lhs, const s16_x& rhs)
{
	return _mm_min_epi16(lhs.value_, rhs.value_);
}

inline s16_x operator+(const s16_x& lhs, const s16_x& rhs)
{
	return s16_x(lhs) += rhs;
}

inline s16_x operator-(const s16_x& lhs, const s16_x& rhs)
{
	return s16_x(lhs) -= rhs;
}

inline s16_x operator>>(const s16_x& lhs, int count)
{		
	return s16_x(lhs) >>= count;
}

inline s16_x operator<<(const s16_x& lhs, int count)
{		
	return s16_x(lhs) <<= count;
}

inline s16_x operator|(const s16_x& lhs, const s16_x& rhs)
{		
	return s16_x(lhs) |= rhs;
}

inline s16_x operator&(const s16_x& lhs, const s16_x& rhs)
{		
	return s16_x(lhs) &= rhs;
}

// s8_x

s8_x::s8_x()
{
}

s8_x::s8_x(const s32_x& other)
	: value_(other.value_)
{
}

s8_x::s8_x(const s16_x& other)
	: value_(other.value_)
{
}

s8_x::s8_x(const u8_x& other)
	: value_(other.value_)
{
}

s8_x::s8_x(const __m128i& value)
	: value_(value)
{
}	

s8_x::s8_x(char b)
	: value_(_mm_set1_epi8(b))
{
}

s8_x::s8_x(char b3,  char b2,  char b1,  char b0)
	: value_(_mm_set_epi8(b3, b2, b1, b0, b3, b2, b1, b0, b3, b2, b1, b0, b3, b2, b1, b0))
{
}

s8_x::s8_x(char b15, char b14, char b13, char b12, 
			char b11, char b10, char b9,  char b8,  
			char b7,  char b6,  char b5,  char b4,  
			char b3,  char b2,  char b1,  char b0)
	: value_(_mm_set_epi8(b15, b14, b13, b12, b11, b10, b9, b8, b7, b6, b5, b4, b3, b2, b1, b0))
{
}
	
s8_x& s8_x::operator+=(const s8_x& other)
{
	value_ = _mm_add_epi8(value_, other.value_);
	return *this;
}

s8_x& s8_x::operator-=(const s8_x& other)
{
	value_ = _mm_sub_epi8(value_, other.value_);
	return *this;
}
									
char s8_x::operator[](int index) const
{
	return value_.m128i_i8[index];
}

char& s8_x::operator[](int index)
{
	return value_.m128i_i8[index];
}
	
s8_x s8_x::upack(const s16_x& lhs, const s16_x& rhs)
{
	return _mm_packus_epi16(lhs.value_, rhs.value_);
}

s16_x s8_x::multiply_add(const s8_x& lhs, const s8_x& rhs)
{		
	return _mm_maddubs_epi16(lhs.value_, rhs.value_);
}

s8_x s8_x::shuffle(const s8_x& lhs, const s8_x& rhs)
{		
	return _mm_shuffle_epi8(lhs.value_, rhs.value_);
}
	
s8_x s8_x::max(const s8_x& lhs, const s8_x& rhs)
{		
	return _mm_max_epi8(lhs.value_, rhs.value_);
}
	
s8_x s8_x::min(const s8_x& lhs, const s8_x& rhs)
{		
	return _mm_min_epi8(lhs.value_, rhs.value_);
}
	
s8_x s8_x::blend(const s8_x& lhs, const s8_x& rhs, const s8_x& mask)
{		
	return _mm_blendv_epi8(lhs.value_, rhs.value_, mask.value_);
}

inline s8_x operator+(const s8_x& lhs, const s8_x& rhs)
{
	return s8_x(lhs) += rhs;
}

inline s8_x operator-(const s8_x& lhs, const s8_x& rhs)
{
	return s8_x(lhs) -= rhs;
}

// u8_x

u8_x::u8_x()
{
}

u8_x::u8_x(const s32_x& other)
	: value_(other.value_)
{
}

u8_x::u8_x(const s16_x& other)
	: value_(other.value_)
{
}

u8_x::u8_x(const s8_x& other)
	: value_(other.value_)
{
}

u8_x::u8_x(const __m128i& value)
	: value_(value)
{
}	

u8_x::u8_x(char b)
	: value_(_mm_set1_epi8(b))
{
}

u8_x::u8_x(char b3,  char b2,  char b1,  char b0)
	: value_(_mm_set_epi8(b3, b2, b1, b0, b3, b2, b1, b0, b3, b2, b1, b0, b3, b2, b1, b0))
{
}

u8_x::u8_x(char b15, char b14, char b13, char b12, 
			char b11, char b10, char b9,  char b8,  
			char b7,  char b6,  char b5,  char b4,  
			char b3,  char b2,  char b1,  char b0)
	: value_(_mm_set_epi8(b15, b14, b13, b12, b11, b10, b9, b8, b7, b6, b5, b4, b3, b2, b1, b0))
{
}
										
char u8_x::operator[](int index) const
{
	return value_.m128i_i8[index];
}

char& u8_x::operator[](int index)
{
	return value_.m128i_i8[index];
}

u8_x u8_x::max(const u8_x& lhs, const u8_x& rhs)
{		
	return _mm_max_epu8(lhs.value_, rhs.value_);
}
	
u8_x u8_x::min(const u8_x& lhs, const u8_x& rhs)
{		
	return _mm_min_epu8(lhs.value_, rhs.value_);
}


// xmm_cast

//template<typename T>
//struct xmm_cast_impl
//{		
//	template<typename U>
//	T operator()(const U& other)
//	{
//		return typename T::xmm_epi_tag(other.value_);
//	}
//};
//
//template<>
//struct xmm_cast_impl<xmm_ps>
//{
//	xmm_ps operator()(const s32_x& other)
//	{
//		return _mm_cvtepi32_ps(other.value_);
//	}
//};
//
//template<typename T, typename U> 
//T xmm_cast(const U& other)
//{
//	return xmm_cast_impl<T>()(other);
//}

}}}}