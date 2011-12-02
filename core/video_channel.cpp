/*
* copyright (c) 2010 Sveriges Television AB <info@casparcg.com>
*
*  This file is part of CasparCG.
*
*    CasparCG is free software: you can redistribute it and/or modify
*    it under the terms of the GNU General Public License as published by
*    the Free Software Foundation, either version 3 of the License, or
*    (at your option) any later version.
*
*    CasparCG is distributed in the hope that it will be useful,
*    but WITHOUT ANY WARRANTY; without even the implied warranty of
*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*    GNU General Public License for more details.

*    You should have received a copy of the GNU General Public License
*    along with CasparCG.  If not, see <http://www.gnu.org/licenses/>.
*
*/

#include "StdAfx.h"

#include "video_channel.h"

#include "video_format.h"

#include "consumer/output.h"
#include "mixer/mixer.h"
#include "mixer/gpu/ogl_device.h"
#include "producer/stage.h"

#include <common/diagnostics/graph.h>
#include <common/env.h>

#include <boost/property_tree/ptree.hpp>

#include <string>

namespace caspar { namespace core {

struct video_channel::implementation : boost::noncopyable
{
	const int						index_;
	video_format_desc				format_desc_;
	const safe_ptr<ogl_device>		ogl_;
	safe_ptr<diagnostics::graph>	graph_;

	safe_ptr<caspar::core::output>	output_;
	safe_ptr<caspar::core::mixer>	mixer_;
	safe_ptr<caspar::core::stage>	stage_;
	
public:
	implementation(int index, const video_format_desc& format_desc, const safe_ptr<ogl_device>& ogl)  
		: index_(index)
		, format_desc_(format_desc)
		, ogl_(ogl)
		, output_(new caspar::core::output(graph_, format_desc, index))
		, mixer_(new caspar::core::mixer(graph_, output_, format_desc, ogl))
		, stage_(new caspar::core::stage(graph_, mixer_, format_desc))	
	{
		graph_->set_text(print());
		diagnostics::register_graph(graph_);

		for(int n = 0; n < std::max(1, env::properties().get("configuration.pipeline-tokens", 2)); ++n)
			stage_->spawn_token();

		CASPAR_LOG(info) << print() << " Successfully Initialized.";
	}
	
	void set_video_format_desc(const video_format_desc& format_desc)
	{
		try
		{
			output_->set_video_format_desc(format_desc);
			mixer_->set_video_format_desc(format_desc);
			ogl_->gc();
		}
		catch(...)
		{
			output_->set_video_format_desc(format_desc_);
			mixer_->set_video_format_desc(format_desc_);
			throw;
		}
		format_desc_ = format_desc;
	}
		
	std::wstring print() const
	{
		return L"video_channel[" + boost::lexical_cast<std::wstring>(index_) + L"|" +  format_desc_.name + L"]";
	}

	boost::property_tree::wptree info() const
	{
		boost::property_tree::wptree info;
		info.put(L"channel.video-mode", format_desc_.name);

		auto& channel_node = info.get_child(L"channel");
		auto stage_info  = stage_->info();
		auto mixer_info  = mixer_->info();
		auto output_info = output_->info();

		BOOST_FOREACH(auto& update, stage_info.get())   
			channel_node.put_child(update.first, update.second);
		BOOST_FOREACH(auto& update, mixer_info.get())   
			channel_node.put_child(update.first, update.second);
		BOOST_FOREACH(auto& update, output_info.get())   
			channel_node.put_child(update.first, update.second);
   
		return info;			   
	}
};

video_channel::video_channel(int index, const video_format_desc& format_desc, const safe_ptr<ogl_device>& ogl) : impl_(new implementation(index, format_desc, ogl)){}
safe_ptr<stage> video_channel::stage() { return impl_->stage_;} 
safe_ptr<mixer> video_channel::mixer() { return impl_->mixer_;} 
safe_ptr<output> video_channel::output() { return impl_->output_;} 
video_format_desc video_channel::get_video_format_desc() const{return impl_->format_desc_;}
void video_channel::set_video_format_desc(const video_format_desc& format_desc){impl_->set_video_format_desc(format_desc);}
boost::property_tree::wptree video_channel::info() const{return impl_->info();}

}}