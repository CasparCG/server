#pragma once

#include "fwd.h"

#include <boost/noncopyable.hpp>
#include <boost/range/iterator_range.hpp>
#include <boost/variant.hpp>
#include <boost/type_traits.hpp>

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

class draw_frame
{
	enum frame_type
	{
		normal_type,
		eof_type,
		empty_type
	};

	//TODO: Move constructor is called when copy constructor should be called.
	template<typename T>
	void init(T&& other, const boost::true_type&)
	{
		impl_ = other.impl_;
		type_ = other.type_;
	}
	
	template<typename T>
	void init(T&& impl, const boost::false_type&)
	{
		impl_ = std::make_shared<T>(std::move(impl));
		type_ = normal_type;
	}

public:
	draw_frame() : type_(empty_type){}
	draw_frame(const draw_frame& other) : impl_(other.impl_), type_(other.type_){}

	template<typename T>
	draw_frame(T&& val)
	{
		init(std::forward<T>(val), boost::is_same<typename std::remove_reference<T>::type, draw_frame>());
	}

	draw_frame(eof_frame&&) : type_(eof_type){}
	draw_frame(empty_frame&&) : type_(empty_type){}
		
	void swap(draw_frame& other);
	draw_frame& operator=(const draw_frame& other);
	draw_frame& operator=(draw_frame&& other);
	draw_frame& operator=(eof_frame&&);
	draw_frame& operator=(empty_frame&&);

	static eof_frame eof();
	static empty_frame empty();
	
	bool operator==(const eof_frame&);
	bool operator!=(const eof_frame&);

	bool operator==(const empty_frame&);
	bool operator!=(const empty_frame&);

	bool operator==(const draw_frame& other);
	bool operator!=(const draw_frame& other);
		
	const std::vector<short>& audio_data() const;

	void begin_write();
	void end_write();
	void draw(frame_shader& shader);
private:		
	draw_frame_impl_ptr impl_;
	frame_type type_;
};

}}