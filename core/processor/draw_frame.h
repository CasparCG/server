#pragma once

#include "fwd.h"

#include <boost/noncopyable.hpp>
#include <boost/range/iterator_range.hpp>
#include <boost/variant.hpp>
#include <boost/type_traits.hpp>
#include <boost/operators.hpp>

#include <memory>
#include <vector>
#include <type_traits>

namespace caspar { namespace core {
		
struct eof_frame{};
struct empty_frame{};
	
struct draw_frame_impl : boost::noncopyable
{	
	virtual const std::vector<short>& audio_data() const = 0;
			
	virtual void begin_write() = 0;
	virtual void end_write() = 0;
	virtual void draw(frame_shader& shader) = 0;
};

class draw_frame : boost::equality_comparable<draw_frame>, boost::equality_comparable<eof_frame>, boost::equality_comparable<empty_frame>
{
	enum frame_tag
	{
		normal_tag,
		eof_tag,
		empty_tag
	};

	static_assert(std::is_abstract<draw_frame_impl>::value, "non-abstract container allows slicing");
public:
	draw_frame();
	draw_frame(const draw_frame& other);
	draw_frame(draw_frame&& other);

	template<typename T>
	draw_frame(T&& impl, typename std::enable_if<std::is_base_of<draw_frame_impl, typename std::remove_reference<T>::type>::value, void>::type* dummy = nullptr)
		: impl_(std::make_shared<T>(std::forward<T>(impl))), type_(normal_tag){}

	draw_frame(eof_frame&&) : type_(eof_tag){}
	draw_frame(empty_frame&&) : type_(empty_tag){}
		
	void swap(draw_frame& other);
	
	template <typename T>
	typename std::enable_if<std::is_base_of<draw_frame_impl, typename std::remove_reference<T>::type>::value, draw_frame&>::type
	operator=(T&& impl)
	{
		impl_ = std::make_shared<T>(std::forward<T>(impl));
		return *this;
	}
	
	draw_frame& operator=(const draw_frame& other);
	draw_frame& operator=(draw_frame&& other);

	draw_frame& operator=(eof_frame&&);
	draw_frame& operator=(empty_frame&&);

	static eof_frame eof();
	static empty_frame empty();
	
	bool operator==(const eof_frame&);
	bool operator==(const empty_frame&);
	bool operator==(const draw_frame& other);
		
	const std::vector<short>& audio_data() const;

	void begin_write();
	void end_write();
	void draw(frame_shader& shader);
private:		
	draw_frame_impl_ptr impl_;
	frame_tag type_;
};

}}