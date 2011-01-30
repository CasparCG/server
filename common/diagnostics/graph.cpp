#include "../stdafx.h"

#include "graph.h"

#pragma warning (disable : 4244)

#include "../concurrency/executor.h"
#include "../utility/timer.h"

#include <SFML/Window.hpp>

#include <boost/foreach.hpp>
#include <boost/circular_buffer.hpp>
#include <boost/range/algorithm_ext/erase.hpp>

#include <numeric>
#include <map>
#include <array>

namespace caspar { namespace diagnostics {

struct drawable
{
	virtual ~drawable(){}
	virtual void draw() = 0;
};

class context
{	
	timer timer_;
	sf::Window window_;
	
	std::list<std::weak_ptr<drawable>> drawables_;
		
	executor executor_;
public:					

	template<typename Func>
	static auto begin_invoke(Func&& func) -> boost::unique_future<decltype(func())> // noexcept
	{	
		return get_instance().executor_.begin_invoke(std::forward<Func>(func));	
	}

	static void register_drawable(const std::shared_ptr<drawable>& drawable)
	{
		begin_invoke([=]
		{
			get_instance().drawables_.push_back(drawable);
		});
	}
	
private:

	void tick()
	{
		sf::Event e;
		while(window_.GetEvent(e)){}		
		glClear(GL_COLOR_BUFFER_BIT);
		render();
		window_.Display();
		timer_.tick(1.0/50.0);
		executor_.begin_invoke([this]{tick();});
	}

	void render()
	{
		glLoadIdentity();
		glTranslated(-1.0f, -1.0f, 0.0f);
		glScaled(2.0f, 2.0f, 1.0f);

		float dy = 1.0/static_cast<float>(std::max<int>(5, drawables_.size()));

		glTranslated(0.0f, (1.0-dy), 0.0f);
		for(auto it = drawables_.begin(); it != drawables_.end();)
		{
			auto drawable = it->lock();
			if(drawable)
			{
				glPushMatrix();
					glScaled(1.0f, dy, 1.0f);
					drawable->draw();
				glPopMatrix();
				glTranslated(0.0f, -dy, 0.0f);
				++it;
			}
			else			
				it = drawables_.erase(it);			
		}		
	}	

	static context& get_instance()
	{
		static context impl;
		return impl;
	}

	context()
	{
		executor_.start();
		executor_.begin_invoke([this]
		{
			window_.Create(sf::VideoMode(600, 1000), "CasparCG Diagnostics");
			window_.SetPosition(0, 0);
			window_.SetActive();
			glEnable(GL_BLEND);
			glEnable(GL_LINE_SMOOTH);
			glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			tick();
		});
	}
};

class line
{
	boost::circular_buffer<float> line_data_;
	std::vector<float> tick_data_;
	color c_;
public:
	line(size_t res = 600)
		: line_data_(res)
		, c_(1.0f, 1.0f, 1.0f){}
	
	void update(float value)
	{
		tick_data_.push_back(value);
	}
	
	void set_color(color c){c_ = c;}
	
	void draw()
	{
		float dx = 1.0f/static_cast<float>(line_data_.capacity());
		float x = static_cast<float>(line_data_.capacity()-line_data_.size())*dx;

		if(!tick_data_.empty())
		{
			float sum = std::accumulate(tick_data_.begin(), tick_data_.end(), 0.0) + std::numeric_limits<float>::min();
			line_data_.push_back(static_cast<float>(sum)/static_cast<float>(tick_data_.size()));
			tick_data_.clear();
		}
		else if(!line_data_.empty())
		{
			line_data_.push_back(line_data_.back());
		}
		
		glBegin(GL_LINE_STRIP);
		glColor4f(c_.red, c_.green, c_.blue, 1.0f);			
		for(size_t n = 0; n < line_data_.size(); ++n)				
			glVertex3f(x+n*dx, std::max(0.05f, std::min(0.95f, line_data_[n]*0.8f + 0.1f)), 0.0f);		
		glEnd();
	}
};
	
class guide
{
	color c_;
	float value_;
public:
	guide() : value_(0.0f){}

	guide(float value, color c) 
		: value_(value)
		, c_(c){}
			
	void draw()
	{		
		glEnable(GL_LINE_STIPPLE);
		glLineStipple(3, 0xAAAA);
		glBegin(GL_LINE_STRIP);
		glColor4f(c_.red, c_.green, c_.blue, 1.0f);				
			glVertex3f(0.0f, value_ * 0.8f + 0.1f, 0.0f);		
			glVertex3f(1.0f, value_ * 0.8f + 0.1f, 0.0f);	
		glEnd();
		glDisable(GL_LINE_STIPPLE);
	}
};

struct graph::implementation : public drawable
{
	std::map<std::string, diagnostics::line> lines_;
	std::map<std::string, diagnostics::guide> guides_;

	implementation(const std::string&)
	{
		guides_["max"] = diagnostics::guide(1.0f, color(0.4f, 0.4f, 0.4f));
		guides_["min"] = diagnostics::guide(0.0f, color(0.4f, 0.4f, 0.4f));
	}

	void update(const std::string& name, float value)
	{
		context::begin_invoke([=]
		{
			lines_[name].update(value);
		});
	}

	void set_color(const std::string& name, color c)
	{
		context::begin_invoke([=]
		{
			lines_[name].set_color(c);
		});
	}
	
	void add_guide(const std::string& name, float value, color c)
	{
		context::begin_invoke([=]
		{
			guides_[name] = diagnostics::guide(value, c);
		});
	}

private:
	void draw()
	{
		glBegin(GL_QUADS);
			glColor4f(1.0f, 1.0f, 1.0f, 0.2f);	
			glVertex2f(1.0f, 0.99f);
			glVertex2f(0.0f, 0.99f);
			glVertex2f(0.0f, 0.01f);	
			glVertex2f(1.0f, 0.01f);	
		glEnd();
		
		for(auto it = guides_.begin(); it != guides_.end(); ++it)
			it->second.draw();

		for(auto it = lines_.begin(); it != lines_.end(); ++it)
			it->second.draw();
	}

	implementation(implementation&);
	implementation& operator=(implementation&);
};
	
graph::graph(const std::string& name) : impl_(new implementation(name))
{
	context::register_drawable(impl_);
}
void graph::update(const std::string& name, float value){impl_->update(name, value);}
void graph::set_color(const std::string& name, color c){impl_->set_color(name, c);}
void graph::add_guide(const std::string& name, float value, color c){impl_->add_guide(name, value, c);}

safe_ptr<graph> create_graph(const std::string& name)
{
	return safe_ptr<graph>(new graph(name));
}

}}