#pragma once

#include <intrin.h>

#include <type_traits>
#include <vector>
#include <tbb/cache_aligned_allocator.h>

typedef std::vector<float, tbb::cache_aligned_allocator<float>> vector_ps;

class xmm_ps
{
	__m128 value_;
public:
	xmm_ps()
	{
	}
	
	xmm_ps(float value_)
		: value_(_mm_set1_ps(value_))
	{
	}
	
	xmm_ps(__m128 value_)
		: value_(value_)
	{
	}
		
	xmm_ps& operator+=(const xmm_ps& other)
	{
		value_ = _mm_add_ps(value_, other.value_);
		return *this;
	}

	xmm_ps& operator-=(const xmm_ps& other)
	{
		value_ = _mm_sub_ps(value_, other.value_);
		return *this;
	}
	
	xmm_ps& operator*=(const xmm_ps& other)
	{
		value_ = _mm_mul_ps(value_, other.value_);
		return *this;
	}
	
	xmm_ps& operator/=(const xmm_ps& other)
	{
		value_ = _mm_div_ps(value_, other.value_);
		return *this;
	}
	
	xmm_ps& horizontal_add(const xmm_ps& other)
	{
		value_ = _mm_hadd_ps(value_, other.value_);
		return *this;
	}

	xmm_ps& horizontal_sub(const xmm_ps& other)
	{
		value_ = _mm_hsub_ps(value_, other.value_);
		return *this;
	}

	xmm_ps unpack_low(const xmm_ps& other)
	{		
		value_ = _mm_unpacklo_ps(value_, other.value_);
		return *this;
	}

	xmm_ps unpack_high(const xmm_ps& other)
	{		
		value_ = _mm_unpackhi_ps(value_, other.value_);
		return *this;
	}
	
	float operator[](int index) const
	{
		return value_.m128_f32[index];
	}

	float& operator[](int index)
	{
		return value_.m128_f32[index];
	}

	static xmm_ps zero()
	{
		return _mm_setzero_ps();
	}

	static xmm_ps load(const float* ptr)
	{
		return _mm_load_ps(ptr);
	}
	
	static xmm_ps loadu(const float* ptr)
	{
		return _mm_loadu_ps(ptr);
	}

	static void stream(const xmm_ps& source, float* dest)
	{
		_mm_stream_ps(dest, source.value_);
	}
		
	static xmm_ps horizontal_add(const xmm_ps& lhs, const xmm_ps& rhs)
	{
		return xmm_ps(lhs).horizontal_add(rhs);
	}

	static xmm_ps horizontal_sub(const xmm_ps& lhs, const xmm_ps& rhs)
	{
		return xmm_ps(lhs).horizontal_sub(rhs);
	}

	static xmm_ps unpack_low(const xmm_ps& lhs, const xmm_ps& rhs)
	{
		return xmm_ps(lhs).unpack_low(rhs);
	}

	static xmm_ps unpack_high(const xmm_ps& lhs, const xmm_ps& rhs)
	{
		return xmm_ps(lhs).unpack_high(rhs);
	}
};
	
inline xmm_ps operator+(const xmm_ps& lhs, const xmm_ps& rhs)
{		
	return xmm_ps(lhs) += rhs;
}

inline xmm_ps operator-(const xmm_ps& lhs, const xmm_ps& rhs)
{		
	return xmm_ps(lhs) -= rhs;
}

inline xmm_ps operator*(const xmm_ps& lhs, const xmm_ps& rhs)
{		
	return xmm_ps(lhs) *= rhs;
}

inline xmm_ps operator/(const xmm_ps& lhs, const xmm_ps& rhs)
{		
	return xmm_ps(lhs) /= rhs;
}

class xmm_epi32
{
	__m128i value_;
	template<typename> friend struct xmm_cast_impl;
public:
	typedef xmm_epi32 xmm_epi_tag;

	xmm_epi32()
	{
	}

	xmm_epi32(__m128i value)
		: value_(value)
	{
	}
	
	xmm_epi32& operator>>=(int count)
	{
		value_ = _mm_srli_epi32(value_, count);
		return *this;
	}
	
	xmm_epi32& operator<<=(int count)
	{
		value_ = _mm_slli_epi32(value_, count);
		return *this;
	}
		
	xmm_epi32& operator|=(const xmm_epi32& other)
	{
		value_ = _mm_or_si128(value_, other.value_);
		return *this;
	}	
	
	xmm_epi32& operator&=(const xmm_epi32& other)
	{
		value_ = _mm_and_si128(value_, other.value_);
		return *this;
	}	

	static xmm_epi32 load(const void* source)
	{
		return _mm_load_si128(reinterpret_cast<const __m128i*>(source));
	}
	
	static xmm_epi32 loadu(const void* source)
	{
		return _mm_loadu_si128(reinterpret_cast<const __m128i*>(source));
	}
		
	int32_t operator[](int index) const
	{
		return value_.m128i_i32[index];
	}

	int32_t& operator[](int index)
	{
		return value_.m128i_i32[index];
	}
};

inline xmm_epi32 operator>>(const xmm_epi32& lhs, int count)
{		
	return xmm_epi32(lhs) >>= count;
}

inline xmm_epi32 operator<<(const xmm_epi32& lhs, int count)
{		
	return xmm_epi32(lhs) <<= count;
}

inline xmm_epi32 operator|(const xmm_epi32& lhs, const xmm_epi32& rhs)
{		
	return xmm_epi32(lhs) |= rhs;
}

inline xmm_epi32 operator&(const xmm_epi32& lhs, const xmm_epi32& rhs)
{		
	return xmm_epi32(lhs) &= rhs;
}

class xmm_epi16
{
	__m128i value_;
	template<typename> friend struct xmm_cast_impl;
	friend xmm_epi32 horizontal_add(const xmm_epi16&);
public:
	typedef xmm_epi16 xmm_epi_tag;

	xmm_epi16()
	{
	}

	xmm_epi16(__m128i value)
		: value_(value)
	{
	}

	xmm_epi16(short value)
		: value_(_mm_set1_epi16(value))
	{
	}

	xmm_epi16& operator+=(const xmm_epi16& other)
	{
		value_ = _mm_add_epi16(value_, other.value_);
		return *this;
	}
	
	xmm_epi16& operator-=(const xmm_epi16& other)
	{
		value_ = _mm_sub_epi16(value_, other.value_);
		return *this;
	}

	xmm_epi16& operator>>=(int count)
	{
		value_ = _mm_srli_epi16(value_, count);
		return *this;
	}
	
	xmm_epi16& operator<<=(int count)
	{
		value_ = _mm_slli_epi16(value_, count);
		return *this;
	}

	xmm_epi16& operator|=(const xmm_epi16& other)
	{
		value_ = _mm_or_si128(value_, other.value_);
		return *this;
	}	
	
	xmm_epi16& operator&=(const xmm_epi16& other)
	{
		value_ = _mm_and_si128(value_, other.value_);
		return *this;
	}	
		
	xmm_epi16 multiply_low(const xmm_epi16& other)
	{		
		value_ = _mm_mullo_epi16(value_, other.value_);
		return *this;
	}

	xmm_epi16 multiply_high(const xmm_epi16& other)
	{		
		value_ = _mm_mulhi_epi16(value_, other.value_);
		return *this;
	}

	xmm_epi16 multiply_low_unsigned(const xmm_epi16& other)
	{		
		value_ = _mm_mullo_epi16(value_, other.value_);
		return *this;
	}

	xmm_epi16 multiply_high_unsigned(const xmm_epi16& other)
	{		
		value_ = _mm_mulhi_epi16(value_, other.value_);
		return *this;
	}
	
	xmm_epi16 and_not(const xmm_epi16& other)
	{		
		value_ = _mm_andnot_si128(other.value_, value_);
		return *this;
	}

	xmm_epi16 unpack_low(const xmm_epi16& other)
	{		
		value_ = _mm_unpacklo_epi16 (value_, other.value_);
		return *this;
	}

	xmm_epi16 unpack_high(const xmm_epi16& other)
	{		
		value_ = _mm_unpackhi_epi16 (value_, other.value_);
		return *this;
	}

	xmm_epi16 pack(const xmm_epi16& other)
	{		
		value_ = _mm_packs_epi16(value_, other.value_);
		return *this;
	}
		
	int16_t operator[](int index) const
	{
		return value_.m128i_i16[index];
	}

	int16_t& operator[](int index)
	{
		return value_.m128i_i16[index];
	}
	
	static xmm_epi16 load(const void* source)
	{
		return _mm_load_si128(reinterpret_cast<const __m128i*>(source));
	}
	
	static xmm_epi16 loadu(const void* source)
	{
		return _mm_loadu_si128(reinterpret_cast<const __m128i*>(source));
	}
	
	static xmm_epi32 horizontal_add(const xmm_epi16& lhs)
	{
		#ifdef SSIM_XOP
				return _mm_haddd_epi16(value_);
		#else
				return _mm_madd_epi16(lhs.value_, _mm_set1_epi16(1));
		#endif
	}

	static xmm_epi16 multiply_low(const xmm_epi16& lhs, const xmm_epi16& rhs)
	{
		return xmm_epi16(lhs).multiply_low(rhs);
	}

	static xmm_epi16 multiply_high(const xmm_epi16& lhs, const xmm_epi16& rhs)
	{
		return xmm_epi16(lhs).multiply_high(rhs);
	}

	static xmm_epi16 multiply_low_unsigned(const xmm_epi16& lhs, const xmm_epi16& rhs)
	{
		return xmm_epi16(lhs).multiply_low_unsigned(rhs);
	}

	static xmm_epi16 multiply_high_unsigned(const xmm_epi16& lhs, const xmm_epi16& rhs)
	{
		return xmm_epi16(lhs).multiply_high_unsigned(rhs);
	}

	static xmm_epi16 unpack_low(const xmm_epi16& lhs, const xmm_epi16& rhs)
	{
		return xmm_epi16(lhs).unpack_low(rhs);
	}

	static xmm_epi16 unpack_high(const xmm_epi16& lhs, const xmm_epi16& rhs)
	{
		return xmm_epi16(lhs).unpack_high(rhs);
	}

	static xmm_epi16 pack(const xmm_epi16& lhs, const xmm_epi16& rhs)
	{
		return xmm_epi16(lhs).pack(rhs);
	}

	static xmm_epi16 and_not(const xmm_epi16& lhs, const xmm_epi16& rhs)
	{
		return xmm_epi16(lhs).and_not(rhs);
	}
};

inline xmm_epi16 operator+(const xmm_epi16& lhs, const xmm_epi16& rhs)
{
	return xmm_epi16(lhs) += rhs;
}

inline xmm_epi16 operator-(const xmm_epi16& lhs, const xmm_epi16& rhs)
{
	return xmm_epi16(lhs) -= rhs;
}

inline xmm_epi16 operator>>(const xmm_epi16& lhs, int count)
{		
	return xmm_epi16(lhs) >>= count;
}

inline xmm_epi16 operator<<(const xmm_epi16& lhs, int count)
{		
	return xmm_epi16(lhs) <<= count;
}

inline xmm_epi16 operator|(const xmm_epi16& lhs, const xmm_epi16& rhs)
{		
	return xmm_epi16(lhs) |= rhs;
}

inline xmm_epi16 operator&(const xmm_epi16& lhs, const xmm_epi16& rhs)
{		
	return xmm_epi16(lhs) &= rhs;
}

class xmm_epi8
{
	__m128i value_;
	template<typename> friend struct xmm_cast_impl;
	friend xmm_epi16 multiply_add(const xmm_epi8&, const xmm_epi8&);
public:
	typedef xmm_epi8 xmm_epi_tag;

	xmm_epi8()
	{
	}

	xmm_epi8(__m128i value)
		: value_(value)
	{
	}
	
	xmm_epi8(char b)
		: value_(_mm_set1_epi8(b))
	{
	}

	xmm_epi8(char b3,  char b2,  char b1,  char b0)
		: value_(_mm_set_epi8(b3, b2, b1, b0, b3, b2, b1, b0, b3, b2, b1, b0, b3, b2, b1, b0))
	{
	}

	xmm_epi8(char b15, char b14, char b13, char b12, 
			 char b11, char b10, char b9,  char b8,  
			 char b7,  char b6,  char b5,  char b4,  
			 char b3,  char b2,  char b1,  char b0)
		: value_(_mm_set_epi8(b15, b14, b13, b12, b11, b10, b9, b8, b7, b6, b5, b4, b3, b2, b1, b0))
	{
	}
	
	xmm_epi8& operator+=(const xmm_epi8& other)
	{
		value_ = _mm_add_epi8(value_, other.value_);
		return *this;
	}

	xmm_epi8& operator-=(const xmm_epi8& other)
	{
		value_ = _mm_sub_epi8(value_, other.value_);
		return *this;
	}
			
	xmm_epi8& shuffle(const xmm_epi8& other)
	{		
		value_ = _mm_shuffle_epi8 (value_, other.value_);
		return *this;
	}

	const xmm_epi8& stream(void* dest) const
	{
		_mm_stream_si128(reinterpret_cast<__m128i*>(dest), value_);
		return *this;
	}
	
	char operator[](int index) const
	{
		return value_.m128i_i8[index];
	}

	char& operator[](int index)
	{
		return value_.m128i_i8[index];
	}

	static const xmm_epi8& stream(const xmm_epi8& source, void* dest)
	{
		source.stream(dest);
		return source;
	}

	static xmm_epi8 load(const void* source)
	{
		return _mm_load_si128(reinterpret_cast<const __m128i*>(source));
	}
	
	static xmm_epi8 loadu(const void* source)
	{
		return _mm_loadu_si128(reinterpret_cast<const __m128i*>(source));
	}

	static xmm_epi16 multiply_add(const xmm_epi8& lhs, const xmm_epi8& rhs)
	{		
		return xmm_epi16(_mm_maddubs_epi16(lhs.value_, rhs.value_));
	}

	static xmm_epi8& shuffle(const xmm_epi8& lhs, const xmm_epi8& rhs)
	{		
		return xmm_epi8(lhs).shuffle(rhs);
	}
};

inline xmm_epi8 operator+(const xmm_epi8& lhs, const xmm_epi8& rhs)
{
	return xmm_epi8(lhs) += rhs;
}

inline xmm_epi8 operator-(const xmm_epi8& lhs, const xmm_epi8& rhs)
{
	return xmm_epi8(lhs) -= rhs;
}

// xmm_cast

template<typename T>
struct xmm_cast_impl
{		
	template<typename U>
	T operator()(const U& other)
	{
		return typename T::xmm_epi_tag(other.value_);
	}
};

template<>
struct xmm_cast_impl<xmm_ps>
{
	xmm_ps operator()(const xmm_epi32& other)
	{
		return _mm_cvtepi32_ps(other.value_);
	}
};

template<typename T, typename U> 
T xmm_cast(const U& other)
{
	return xmm_cast_impl<T>()(other);
}


