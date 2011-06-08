#include "stdAfx.h"

#include "video_channel_context.h"

namespace caspar { namespace core {

video_channel_context::video_channel_context(int index, ogl_device& ogl, const video_format_desc& format_desc) 
	: index_(index)
	, format_desc_(format_desc)
	, execution_(print() + L"/execution")
	, destruction_(print() + L"/destruction")
	, ogl_(ogl)
{
	execution_.set_priority_class(above_normal_priority_class);
	destruction_.set_priority_class(below_normal_priority_class);
}

const int video_channel_context::index() const {return index_;}

video_format_desc video_channel_context::get_format_desc()
{
	tbb::spin_rw_mutex::scoped_lock lock(mutex_, false);
	return format_desc_;
}

void video_channel_context::set_format_desc(const video_format_desc& format_desc)
{
	tbb::spin_rw_mutex::scoped_lock lock(mutex_, true);
	format_desc_ = format_desc;
}

executor& video_channel_context::execution() {return execution_;}
executor& video_channel_context::destruction() {return destruction_;}
ogl_device& video_channel_context::ogl() { return ogl_;}

std::wstring video_channel_context::print() const
{
	return L"video_channel[" + boost::lexical_cast<std::wstring>(index_+1) + L"|" +  format_desc_.name + L"]";
}

}}