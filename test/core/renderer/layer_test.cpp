#include <gtest/gtest.h>

#include <core/frame/gpu_frame.h>
#include <core/renderer/layer.h>
#include <core/producer/frame_producer.h>
#include <common/exception/exceptions.h>

using namespace caspar;
using namespace caspar::core;

struct mock_frame_producer : public frame_producer
{
	gpu_frame_ptr get_frame()
	{ return std::make_shared<gpu_frame>(0, 0); }
	std::shared_ptr<frame_producer> get_following_producer() const 
	{ return nullptr; }
	void set_leading_producer(const std::shared_ptr<frame_producer>&) 
	{}
	const frame_format_desc& get_frame_format_desc() const
	{return frame_format_desc();}
	void initialize(const frame_factory_ptr& factory)
	{}
};

TEST(layer_test, load_nullptr) 
{
	core::renderer::layer layer;

	ASSERT_THROW(layer.load(nullptr), null_argument);
}

TEST(layer_test, load)
{	
	core::renderer::layer layer;
	auto background = std::make_shared<mock_frame_producer>();

	layer.load(background);

	auto frame = layer.get_frame();

	ASSERT_TRUE(layer.active() == nullptr);
	ASSERT_TRUE(layer.background() == background);
	ASSERT_TRUE(frame == nullptr);
}

TEST(layer_test, load_auto_play)
{	
	core::renderer::layer layer;
	auto active = std::make_shared<mock_frame_producer>();

	layer.load(active, renderer::load_option::auto_play);
	
	auto frame = layer.get_frame();

	ASSERT_TRUE(layer.active() == active);
	ASSERT_TRUE(layer.background() == nullptr);
	ASSERT_TRUE(frame != nullptr);
}

TEST(layer_test, load_active)
{	
	core::renderer::layer layer;
	auto active = std::make_shared<mock_frame_producer>();
	auto background = std::make_shared<mock_frame_producer>();

	layer.load(active, renderer::load_option::auto_play);
	layer.load(background);
	
	auto frame = layer.get_frame();

	ASSERT_TRUE(layer.active() == active);
	ASSERT_TRUE(layer.background() == background);
	ASSERT_TRUE(frame != nullptr);
}

TEST(layer_test, load_preview)
{	
	core::renderer::layer layer;
	auto background = std::make_shared<mock_frame_producer>();

	layer.load(background, renderer::load_option::preview);

	auto frame = layer.get_frame();

	ASSERT_TRUE(layer.active() == nullptr);
	ASSERT_TRUE(layer.background() == background);
	ASSERT_TRUE(frame != nullptr);
	ASSERT_TRUE(frame->audio_data().empty());
}

TEST(layer_test, load_preview_active)
{	
	core::renderer::layer layer;
	auto active = std::make_shared<mock_frame_producer>();
	auto background = std::make_shared<mock_frame_producer>();

	layer.load(active, renderer::load_option::auto_play);
	layer.load(background, renderer::load_option::preview);
	
	auto frame = layer.get_frame();

	ASSERT_TRUE(layer.active() == nullptr);
	ASSERT_TRUE(layer.background() == background);
	ASSERT_TRUE(frame != nullptr);
}

TEST(layer_test, load_play)
{	
	core::renderer::layer layer;
	auto active = std::make_shared<mock_frame_producer>();

	layer.load(active);
	layer.play();

	auto frame = layer.get_frame();

	ASSERT_TRUE(layer.active() == active);
	ASSERT_TRUE(layer.background() == nullptr);
	ASSERT_TRUE(frame != nullptr);
}
