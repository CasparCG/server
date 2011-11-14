#include "stdAfx.h"

#include "video_channel_context.h"

namespace caspar { namespace core {

struct video_channel_context::implementation
{		
	mutable tbb::spin_rw_mutex	mutex_;
	const int					index_;
	video_format_desc			format_desc_;
	executor					execution_;
	safe_ptr<ogl_device>		ogl_;

	implementation(int index, const safe_ptr<ogl_device>& ogl, const video_format_desc& format_desc)
		: index_(index)
		, format_desc_(format_desc)
		, execution_(print() + L"/execution")
		, ogl_(ogl)
	{
		execution_.set_priority_class(above_normal_priority_class);
	}

	std::wstring print() const
	{
		return L"video_channel[" + boost::lexical_cast<std::wstring>(index_+1) + L"|" +  format_desc_.name + L"]";
	}
};

video_channel_context::video_channel_context(int index, const safe_ptr<ogl_device>& ogl, const video_format_desc& format_desc) 
	: impl_(new implementation(index, ogl, format_desc))
{
}

const int video_channel_context::index() const {return impl_->index_;}

video_format_desc video_channel_context::get_format_desc()
{
	tbb::spin_rw_mutex::scoped_lock lock(impl_->mutex_, false);
	return impl_->format_desc_;
}

void video_channel_context::set_format_desc(const video_format_desc& format_desc)
{
	tbb::spin_rw_mutex::scoped_lock lock(impl_->mutex_, true);
	impl_->format_desc_ = format_desc;
}

executor& video_channel_context::execution() {return impl_->execution_;}
safe_ptr<ogl_device> video_channel_context::ogl() { return impl_->ogl_;}

std::wstring video_channel_context::print() const
{
	return impl_->print();
}

}}