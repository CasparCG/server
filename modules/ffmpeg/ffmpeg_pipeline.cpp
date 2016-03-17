/*
* Copyright (c) 2011 Sveriges Television AB <info@casparcg.com>
*
* This file is part of CasparCG (www.casparcg.com).
*
* CasparCG is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* CasparCG is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with CasparCG. If not, see <http://www.gnu.org/licenses/>.
*
* Author: Helge Norberg, helge.norberg@svt.se
*/

#include "StdAfx.h"

#include "ffmpeg_pipeline.h"
#include "ffmpeg_pipeline_backend.h"
#include "ffmpeg_pipeline_backend_internal.h"

#include <core/frame/draw_frame.h>
#include <core/video_format.h>

namespace caspar { namespace ffmpeg {

ffmpeg_pipeline::ffmpeg_pipeline()
	: impl_(create_internal_pipeline())
{
}

ffmpeg_pipeline			ffmpeg_pipeline::graph(spl::shared_ptr<caspar::diagnostics::graph> g)													{ impl_->graph(std::move(g)); return *this; }
ffmpeg_pipeline			ffmpeg_pipeline::from_file(std::string filename)																		{ impl_->from_file(std::move(filename)); return *this; }
ffmpeg_pipeline			ffmpeg_pipeline::from_memory_only_audio(int num_channels, int samplerate)												{ impl_->from_memory_only_audio(num_channels, samplerate); return *this; }
ffmpeg_pipeline			ffmpeg_pipeline::from_memory_only_video(int width, int height, boost::rational<int> framerate)							{ impl_->from_memory_only_video(width, height, std::move(framerate)); return *this; }
ffmpeg_pipeline			ffmpeg_pipeline::from_memory(int num_channels, int samplerate, int width, int height, boost::rational<int> framerate)	{ impl_->from_memory(num_channels, samplerate, width, height, std::move(framerate)); return *this; }
ffmpeg_pipeline			ffmpeg_pipeline::start_frame(std::uint32_t frame)																		{ impl_->start_frame(frame); return *this; }
std::uint32_t			ffmpeg_pipeline::start_frame() const																					{ return impl_->start_frame(); }
ffmpeg_pipeline			ffmpeg_pipeline::length(std::uint32_t frames)																			{ impl_->length(frames); return *this; }
std::uint32_t			ffmpeg_pipeline::length() const																							{ return impl_->length(); }
ffmpeg_pipeline			ffmpeg_pipeline::seek(std::uint32_t frame)																				{ impl_->seek(frame); return *this; }
ffmpeg_pipeline			ffmpeg_pipeline::loop(bool value)																						{ impl_->loop(value); return *this; }
bool					ffmpeg_pipeline::loop() const																							{ return impl_->loop(); }
std::string				ffmpeg_pipeline::source_filename() const																				{ return impl_->source_filename(); }
ffmpeg_pipeline			ffmpeg_pipeline::vfilter(std::string filter)																			{ impl_->vfilter(std::move(filter)); return *this; }
ffmpeg_pipeline			ffmpeg_pipeline::afilter(std::string filter)																			{ impl_->afilter(std::move(filter)); return *this; }
int						ffmpeg_pipeline::width() const																							{ return impl_->width(); }
int						ffmpeg_pipeline::height() const																							{ return impl_->height(); }
boost::rational<int>	ffmpeg_pipeline::framerate() const																						{ return impl_->framerate(); }
bool					ffmpeg_pipeline::progressive() const																					{ return impl_->progressive(); }
ffmpeg_pipeline			ffmpeg_pipeline::to_memory(spl::shared_ptr<core::frame_factory> factory, core::video_format_desc format)				{ impl_->to_memory(std::move(factory), std::move(format)); return *this; }
ffmpeg_pipeline			ffmpeg_pipeline::to_file(std::string filename)																			{ impl_->to_file(std::move(filename)); return *this; }
ffmpeg_pipeline			ffmpeg_pipeline::vcodec(std::string codec)																				{ impl_->vcodec(std::move(codec)); return *this; }
ffmpeg_pipeline			ffmpeg_pipeline::acodec(std::string codec)																				{ impl_->acodec(std::move(codec)); return *this; }
ffmpeg_pipeline			ffmpeg_pipeline::format(std::string fmt)																				{ impl_->format(std::move(fmt)); return *this; }
ffmpeg_pipeline			ffmpeg_pipeline::start()																								{ impl_->start(); return *this; }
bool					ffmpeg_pipeline::try_push_audio(caspar::array<const std::int32_t> data)													{ return impl_->try_push_audio(std::move(data)); }
bool					ffmpeg_pipeline::try_push_video(caspar::array<const std::uint8_t> data)													{ return impl_->try_push_video(std::move(data)); }
core::draw_frame		ffmpeg_pipeline::try_pop_frame()																						{ return impl_->try_pop_frame(); }
std::uint32_t			ffmpeg_pipeline::last_frame() const																						{ return impl_->last_frame(); }
bool					ffmpeg_pipeline::started() const																						{ return impl_->started(); }
void					ffmpeg_pipeline::stop()																									{ impl_->stop(); }

}}
