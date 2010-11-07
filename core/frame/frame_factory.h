#pragma once

#include "gpu_frame.h"
#include "frame_format.h"

#include <memory>
#include <array>

namespace caspar { namespace core { 

////////////////////////////////////////////////////////////////////////////////////////////////////
/// \struct	frame_factory
///
/// \brief	Factory interface used to create frames.
////////////////////////////////////////////////////////////////////////////////////////////////////
struct frame_factory
{
	////////////////////////////////////////////////////////////////////////////////////////////////////
	/// \fn	virtual ~frame_factory()
	///
	/// \brief	Destructor. 
	////////////////////////////////////////////////////////////////////////////////////////////////////
	virtual ~frame_factory(){}

	////////////////////////////////////////////////////////////////////////////////////////////////////
	/// \fn	virtual void release_frames(void* tag) = 0;
	///
	/// \brief	Releases the frame pool associated with the provided tag. 
	///
	/// \param  tag		Tag associated with the source frame pool. 
	////////////////////////////////////////////////////////////////////////////////////////////////////
	virtual void release_frames(void* tag) = 0;

	////////////////////////////////////////////////////////////////////////////////////////////////////
	/// \fn	virtual gpu_frame_ptr create_frame(size_t width, size_t height, void* tag) = 0;
	///
	/// \brief	Creates a frame from a pool associated with the provided tag. 
	/// 		Frames are pooled on destruction and need to be released with *release_frames*. 
	///
	/// \param	width		The width. 
	/// \param	height		The height. 
	/// \param  tag			Tag associated with the source frame pool. 
	///
	/// \return	. 
	////////////////////////////////////////////////////////////////////////////////////////////////////
	virtual gpu_frame_ptr create_frame(size_t width, size_t height, void* tag) = 0;

	////////////////////////////////////////////////////////////////////////////////////////////////////
	/// \fn	virtual gpu_frame_ptr create_frame(const gpu_frame_desc& desc, void* tag) = 0;
	///
	/// \brief	Creates a frame from a pool associated with the provided tag. 
	/// 		Frames are pooled on destruction and need to be released with *release_frames*. 
	///
	/// \param	desc		Information describing the frame. 
	/// \param  tag			Tag associated with the source frame pool. 
	///
	/// \return	. 
	////////////////////////////////////////////////////////////////////////////////////////////////////
	virtual gpu_frame_ptr create_frame(const gpu_frame_desc& desc, void* tag) = 0;

	////////////////////////////////////////////////////////////////////////////////////////////////////
	/// \fn	gpu_frame_ptr create_frame(const frame_format_desc format_desc, void* tag)
	///
	/// \brief	Creates a frame from a pool associated with the provided tag. 
	/// 		Frames are pooled on destruction and need to be released with *release_frames*. 
	///
	/// \param	format_desc	Information describing the frame format. 
	/// \param  tag			Tag associated with the source frame pool. 
	///
	/// \return	. 
	////////////////////////////////////////////////////////////////////////////////////////////////////
	gpu_frame_ptr create_frame(const frame_format_desc format_desc, void* tag)
	{
		return create_frame(format_desc.width, format_desc.height, tag);
	}
};

typedef std::shared_ptr<frame_factory> frame_factory_ptr;

}}