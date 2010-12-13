#pragma once

#include "fwd.h"

#include <boost/noncopyable.hpp>
#include <boost/range/iterator_range.hpp>
#include <boost/variant.hpp>
#include <boost/operators.hpp>

#include <memory>
#include <vector>
#include <type_traits>

namespace caspar { namespace core {
		
namespace detail
{

class draw_frame_impl : boost::noncopyable
{	
public:
	virtual ~draw_frame_impl(){}

	virtual const std::vector<short>& audio_data() const = 0;		
	virtual void begin_write() = 0;
	virtual void end_write() = 0;
	virtual void draw(frame_shader& shader) = 0;
};

struct draw_frame_access;

}

struct eof_frame{};
struct empty_frame{};

class draw_frame : boost::equality_comparable<draw_frame>, boost::equality_comparable<eof_frame>, boost::equality_comparable<empty_frame>
{
public:
	draw_frame();
	draw_frame(const draw_frame& other);
	draw_frame(draw_frame&& other);
	draw_frame(eof_frame);
	draw_frame(empty_frame);

	template<typename T>
	draw_frame(T&& impl, typename std::enable_if<std::is_base_of<detail::draw_frame_impl, typename std::remove_reference<T>::type>::value, void>::type* dummy = nullptr)
		: impl_(std::make_shared<T>(std::forward<T>(impl))) {dummy;}
			
	void swap(draw_frame& other);
	
	template <typename T>
	typename std::enable_if<std::is_base_of<detail::draw_frame_impl, typename std::remove_reference<T>::type>::value, draw_frame&>::type
	operator=(T&& impl)
	{
		draw_frame temp(std::forward<T>(impl));
		temp.swap(*this);
		return *this;
	}
	
	draw_frame& operator=(const draw_frame& other);
	draw_frame& operator=(draw_frame&& other);

	draw_frame& operator=(eof_frame&&);
	draw_frame& operator=(empty_frame&&);
	
	bool operator==(const draw_frame& other);
	bool operator==(const eof_frame&);
	bool operator==(const empty_frame&);

	static eof_frame eof();
	static empty_frame empty();
		
	const std::vector<short>& audio_data() const;
private:		
	friend struct detail::draw_frame_access;

	void begin_write();
	void end_write();
	void draw(frame_shader& shader);

	std::shared_ptr<detail::draw_frame_impl> impl_;
};

namespace detail
{
	struct draw_frame_access
	{
		static void begin_write(draw_frame& frame){frame.begin_write();}
		static void end_write(draw_frame& frame){frame.end_write();}
		static void draw(draw_frame& frame, frame_shader& shader){frame.draw(shader);}
	};
}

}}