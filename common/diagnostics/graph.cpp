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
* Author: Robert Nagy, ronag89@gmail.com
*/

#include "../stdafx.h"

#include "graph.h"

#pragma warning (disable : 4244)

#include "../executor.h"
#include "../lock.h"
#include "../env.h"

#include <SFML/Graphics.hpp>

#include <boost/foreach.hpp>
#include <boost/optional.hpp>
#include <boost/circular_buffer.hpp>
#include <boost/range/algorithm_ext/erase.hpp>

#include <tbb/concurrent_unordered_map.h>
#include <tbb/atomic.h>
#include <tbb/spin_mutex.h>

#include <array>
#include <numeric>
#include <tuple>

namespace caspar { namespace diagnostics {
		
int color(float r, float g, float b, float a)
{
	int code = 0;
	code |= static_cast<int>(r*255.0f+0.5f) << 24;
	code |= static_cast<int>(g*255.0f+0.5f) << 16;
	code |= static_cast<int>(b*255.0f+0.5f) <<  8;
	code |= static_cast<int>(a*255.0f+0.5f) <<  0;
	return code;
}

std::tuple<float, float, float, float> color(int code)
{
	float r = static_cast<float>((code >> 24) & 255)/255.0f;
	float g = static_cast<float>((code >> 16) & 255)/255.0f;
	float b = static_cast<float>((code >>  8) & 255)/255.0f;
	float a = static_cast<float>((code >>  0) & 255)/255.0f;
	return std::make_tuple(r, g, b, a);
}

struct drawable : public sf::Drawable
{
	virtual ~drawable(){}
	virtual void render(sf::RenderTarget& target) = 0;
	virtual void Render(sf::RenderTarget& target) const { const_cast<drawable*>(this)->render(target);}
};

class context : public drawable
{	
	std::unique_ptr<sf::RenderWindow> window_;
	
	std::list<std::weak_ptr<drawable>> drawables_;
		
	executor executor_;
public:					

	static void register_drawable(const std::shared_ptr<drawable>& drawable)
	{
		if(!drawable)
			return;

		get_instance().executor_.begin_invoke([=]
		{
			get_instance().do_register_drawable(drawable);
		}, task_priority::high_priority);
	}

	static void show(bool value)
	{
		get_instance().executor_.begin_invoke([=]
		{	
			get_instance().do_show(value);
		}, task_priority::high_priority);
	}
				
private:
	context() : executor_(L"diagnostics")
	{
		executor_.begin_invoke([=]
		{			
			SetThreadPriority(GetCurrentThread(), BELOW_NORMAL_PRIORITY_CLASS);
		});
	}

	void do_show(bool value)
	{
		if(value)
		{
			if(!window_)
			{
				window_.reset(new sf::RenderWindow(sf::VideoMode(750, 750), "CasparCG Diagnostics"));
				window_->SetPosition(0, 0);
				window_->SetActive();
				glEnable(GL_BLEND);
				glEnable(GL_LINE_SMOOTH);
				glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
				glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
				tick();
			}
		}
		else
			window_.reset();
	}

	void tick()
	{
		if(!window_)
			return;

		sf::Event e;
		while(window_->GetEvent(e))
		{
			if(e.Type == sf::Event::Closed)
			{
				window_.reset();
				return;
			}
		}		
		glClear(GL_COLOR_BUFFER_BIT);
		window_->Draw(*this);
		window_->Display();
		boost::this_thread::sleep(boost::posix_time::milliseconds(10));
		executor_.begin_invoke([this]{tick();});
	}

	void render(sf::RenderTarget& target)
	{
		auto count = std::max<size_t>(8, drawables_.size());
		float target_dy = 1.0f/static_cast<float>(count);

		float last_y = 0.0f;
		int n = 0;
		for(auto it = drawables_.begin(); it != drawables_.end(); ++n)
		{
			auto drawable = it->lock();
			if(drawable)
			{
				drawable->SetScale(static_cast<float>(window_->GetWidth()), static_cast<float>(target_dy*window_->GetHeight()));
				float target_y = std::max(last_y, static_cast<float>(n * window_->GetHeight())*target_dy);
				drawable->SetPosition(0.0f, target_y);			
				target.Draw(*drawable);				
				++it;		
			}
			else	
				it = drawables_.erase(it);			
		}		
	}	
	
	void do_register_drawable(const std::shared_ptr<drawable>& drawable)
	{
		drawables_.push_back(drawable);
		auto it = drawables_.begin();
		while(it != drawables_.end())
		{
			if(it->lock())
				++it;
			else	
				it = drawables_.erase(it);			
		}
	}
	
	static context& get_instance()
	{
		static context impl;
		return impl;
	}
};

class line : public drawable
{
	boost::circular_buffer<std::pair<float, bool>> line_data_;

	tbb::atomic<float>	tick_data_;
	tbb::atomic<bool>	tick_tag_;
	tbb::atomic<int>	color_;
public:
	line(size_t res = 1200)
		: line_data_(res)
	{
		tick_data_	= -1.0f;
		color_		= 0xFFFFFFFF;
		tick_tag_	= false;

		line_data_.push_back(std::make_pair(-1.0f, false));
	}
	
	void set_value(float value)
	{
		tick_data_ = value;
	}
	
	void set_tag()
	{
		tick_tag_ = true;
	}
		
	void set_color(int color)
	{
		color_ = color;
	}

	int get_color()
	{
		return color_;
	}
		
	void render(sf::RenderTarget&)
	{
		float dx = 1.0f/static_cast<float>(line_data_.capacity());
		float x = static_cast<float>(line_data_.capacity()-line_data_.size())*dx;

		line_data_.push_back(std::make_pair(tick_data_, tick_tag_));		
		tick_tag_   = false;
				
		glBegin(GL_LINE_STRIP);
		auto c = color(color_);
		glColor4f(std::get<0>(c), std::get<1>(c), std::get<2>(c), 0.8f);		
		for(size_t n = 0; n < line_data_.size(); ++n)		
			if(line_data_[n].first > -0.5)
				glVertex3d(x+n*dx, std::max(0.05, std::min(0.95, (1.0f-line_data_[n].first)*0.8 + 0.1f)), 0.0);		
		glEnd();
				
		glEnable(GL_LINE_STIPPLE);
		glLineStipple(3, 0xAAAA);
		for(size_t n = 0; n < line_data_.size(); ++n)	
		{
			if(line_data_[n].second)
			{
				glBegin(GL_LINE_STRIP);			
					glVertex3f(x+n*dx, 0.0f, 0.0f);				
					glVertex3f(x+n*dx, 1.0f, 0.0f);		
				glEnd();
			}
		}
		glDisable(GL_LINE_STIPPLE);
	}
};

struct graph::impl : public drawable
{
	tbb::concurrent_unordered_map<std::string, diagnostics::line> lines_;

	tbb::spin_mutex mutex_;
	std::wstring text_;

	impl()
	{
	}
		
	void set_text(const std::wstring& value)
	{
		auto temp = value;
		lock(mutex_, [&]
		{
			text_ = std::move(temp);
		});
	}

	void set_value(const std::string& name, double value)
	{
		lines_[name].set_value(value);
	}

	void set_tag(const std::string& name)
	{
		lines_[name].set_tag();
	}

	void set_color(const std::string& name, int color)
	{
		lines_[name].set_color(color);
	}
		
private:
	void render(sf::RenderTarget& target)
	{
		const size_t text_size = 15;
		const size_t text_margin = 2;
		const size_t text_offset = (text_size+text_margin*2)*2;

		std::wstring text_str;
		{
			tbb::spin_mutex::scoped_lock lock(mutex_);
			text_str = text_;
		}

		sf::String text(text_str.c_str(), sf::Font::GetDefaultFont(), text_size);
		text.SetStyle(sf::String::Italic);
		text.Move(text_margin, text_margin);
		
		glPushMatrix();
			glScaled(1.0f/GetScale().x, 1.0f/GetScale().y, 1.0f);
			target.Draw(text);
			float x_offset = text_margin;
			for(auto it = lines_.begin(); it != lines_.end(); ++it)
			{						
				sf::String line_text(it->first, sf::Font::GetDefaultFont(), text_size);
				line_text.SetPosition(x_offset, text_margin+text_offset/2);
				auto c = it->second.get_color();
				line_text.SetColor(sf::Color((c >> 24) & 255, (c >> 16) & 255, (c >> 8) & 255, (c >> 0) & 255));
				target.Draw(line_text);
				x_offset = line_text.GetRect().Right + text_margin*2;
			}

			glDisable(GL_TEXTURE_2D);
		glPopMatrix();

		glBegin(GL_QUADS);
			glColor4f(1.0f, 1.0f, 1.0f, 0.2f);	
			glVertex2f(1.0f, 0.99f);
			glVertex2f(0.0f, 0.99f);
			glVertex2f(0.0f, 0.01f);	
			glVertex2f(1.0f, 0.01f);	
		glEnd();

		glPushMatrix();
			glTranslated(0.0f, text_offset/GetScale().y, 1.0f);
			glScaled(1.0f, 1.0-text_offset/GetScale().y, 1.0f);
		
			glEnable(GL_LINE_STIPPLE);
			glLineStipple(3, 0xAAAA);
			glColor4f(1.0f, 1.0f, 1.9f, 0.5f);	
			glBegin(GL_LINE_STRIP);		
				glVertex3f(0.0f, (1.0f-0.5f) * 0.8f + 0.1f, 0.0f);		
				glVertex3f(1.0f, (1.0f-0.5f) * 0.8f + 0.1f, 0.0f);	
			glEnd();
			glBegin(GL_LINE_STRIP);		
				glVertex3f(0.0f, (1.0f-0.0f) * 0.8f + 0.1f, 0.0f);		
				glVertex3f(1.0f, (1.0f-0.0f) * 0.8f + 0.1f, 0.0f);	
			glEnd();
			glBegin(GL_LINE_STRIP);		
				glVertex3f(0.0f, (1.0f-1.0f) * 0.8f + 0.1f, 0.0f);		
				glVertex3f(1.0f, (1.0f-1.0f) * 0.8f + 0.1f, 0.0f);	
			glEnd();
			glDisable(GL_LINE_STIPPLE);

			//target.Draw(diagnostics::guide(1.0f, color(1.0f, 1.0f, 1.0f, 0.6f)));
			//target.Draw(diagnostics::guide(0.0f, color(1.0f, 1.0f, 1.0f, 0.6f)));

			for(auto it = lines_.begin(); it != lines_.end(); ++it)		
				target.Draw(it->second);
		
		glPopMatrix();
	}

	impl(impl&);
	impl& operator=(impl&);
};
	
graph::graph() : impl_(new impl())
{
}

void graph::set_text(const std::wstring& value){impl_->set_text(value);}
void graph::set_value(const std::string& name, double value){impl_->set_value(name, value);}
void graph::set_color(const std::string& name, int color){impl_->set_color(name, color);}
void graph::set_tag(const std::string& name){impl_->set_tag(name);}

void register_graph(const spl::shared_ptr<graph>& graph)
{
	context::register_drawable(graph->impl_);
}

void show_graphs(bool value)
{
	context::show(value);
}

//namespace v2
//{	
//	
//struct line::impl
//{
//	std::wstring name_;
//	boost::circular_buffer<data> ticks_;
//
//	impl(const std::wstring& name) 
//		: name_(name)
//		, ticks_(1024){}
//	
//	void set_value(float value)
//	{
//		ticks_.push_back();
//		ticks_.back().value = value;
//	}
//
//	void set_value(float value)
//	{
//		ticks_.clear();
//		set_value(value);
//	}
//};
//
//line::line(){}
//line::line(const std::wstring& name) : impl_(new impl(name)){}
//std::wstring line::print() const {return impl_->name_;}
//void line::set_value(float value){impl_->set_value(value);}
//void line::set_value(float value){impl_->set_value(value);}
//boost::circular_buffer<data>& line::ticks() { return impl_->ticks_;}
//
//struct graph::impl
//{
//	std::map<std::wstring, line> lines_;
//	color						 color_;
//	printer						 printer_;
//
//	impl(const std::wstring& name) 
//		: printer_([=]{return name;}){}
//
//	impl(const printer& parent_printer) 
//		: printer_(parent_printer){}
//	
//	void set_value(const std::wstring& name, float value)
//	{
//		auto it = lines_.find(name);
//		if(it == lines_.end())
//			it = lines_.insert(std::make_pair(name, line(name))).first;
//
//		it->second.set_value(value);
//	}
//
//	void set_value(const std::wstring& name, float value)
//	{
//		auto it = lines_.find(name);
//		if(it == lines_.end())
//			it = lines_.insert(std::make_pair(name, line(name))).first;
//
//		it->second.set_value(value);
//	}
//	
//	void set_color(const std::wstring& name, color color)
//	{
//		color_ = color;
//	}
//
//	std::map<std::wstring, line>& get_lines()
//	{
//		return lines_;
//	}
//	
//	color get_color() const
//	{
//		return color_;
//	}
//
//	std::wstring print() const
//	{
//		return printer_ ? printer_() : L"graph";
//	}
//};
//	
//graph::graph(const std::wstring& name) : impl_(new impl(name)){}
//graph::graph(const printer& parent_printer) : impl_(new impl(parent_printer)){}
//void graph::set_value(const std::wstring& name, float value){impl_->set_value(name, value);}
//void graph::set_value(const std::wstring& name, float value){impl_->set_value(name, value);}
//void graph::set_color(const std::wstring& name, color c){impl_->set_color(name, c);}
//color graph::get_color() const {return impl_->get_color();}
//std::wstring graph::print() const {return impl_->print();}
//
//spl::shared_ptr<graph> graph::clone() const 
//{
//	spl::shared_ptr<graph> clone(new graph(std::wstring(L"")));
//	clone->impl_->printer_ = impl_->printer_;
//	clone->impl_->lines_ = impl_->lines_;
//	clone->impl_->color_ = impl_->color_;	
//}
//
//std::map<std::wstring, line>& graph::get_lines() {impl_->get_lines();}
//
//std::vector<spl::shared_ptr<graph>> g_graphs;
//
//spl::shared_ptr<graph> create_graph(const std::string& name)
//{
//	g_graphs.push_back(spl::make_shared<graph>(name));
//	return g_graphs.back();
//}
//
//spl::shared_ptr<graph> create_graph(const printer& parent_printer)
//{
//	g_graphs.push_back(spl::make_shared<graph>(parent_printer));
//	return g_graphs.back();
//}
//
//static std::vector<spl::shared_ptr<graph>> get_all_graphs()
//{
//	std::vector<spl::shared_ptr<graph>> graphs;
//	BOOST_FOREACH(auto& graph, g_graphs)
//		graphs.push_back(graph->clone());
//
//	return graphs;
//}
//
//}

}}