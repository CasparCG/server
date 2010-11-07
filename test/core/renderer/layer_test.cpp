#include <gtest/gtest.h>

#include "../mock/mock_frame_producer.h"

#include <core/frame/gpu_frame.h>
#include <core/renderer/layer.h>
#include <core/producer/frame_producer.h>
#include <common/exception/exceptions.h>

using namespace caspar;
using namespace caspar::core;

TEST(layer, load_nullptr) 
{
	core::renderer::layer layer;

	ASSERT_THROW(layer.load(nullptr), null_argument);
}

TEST(layer, load)
{	
	core::renderer::layer layer;
	auto background = std::make_shared<mock_frame_producer>();

	layer.load(background);
	
	ASSERT_TRUE(layer.render_frame() == nullptr);
	ASSERT_TRUE(layer.active() == nullptr);
	ASSERT_TRUE(layer.background() == background);
}

TEST(layer, load_auto_play)
{	
	core::renderer::layer layer;
	auto active = std::make_shared<mock_frame_producer>();

	layer.load(active, renderer::load_option::auto_play);
		
	ASSERT_TRUE(layer.render_frame() != nullptr);
	ASSERT_TRUE(layer.active() == active);
	ASSERT_TRUE(layer.background() == nullptr);
}

TEST(layer, load_active)
{	
	core::renderer::layer layer;
	auto active = std::make_shared<mock_frame_producer>();
	auto background = std::make_shared<mock_frame_producer>();

	layer.load(active, renderer::load_option::auto_play);
	layer.load(background);
	
	ASSERT_TRUE(layer.render_frame() != nullptr);
	ASSERT_TRUE(layer.active() == active);
	ASSERT_TRUE(layer.background() == background);
}

TEST(layer, load_preview)
{	
	core::renderer::layer layer;
	auto background = std::make_shared<mock_frame_producer>();

	layer.load(background, renderer::load_option::preview);

	auto frame = layer.render_frame();
	
	ASSERT_TRUE(frame != nullptr);
	ASSERT_TRUE(layer.render_frame() == frame);
	ASSERT_TRUE(layer.active() == nullptr);
	ASSERT_TRUE(layer.background() == background);
	ASSERT_TRUE(frame->audio_data().empty());
}

TEST(layer, load_preview_active)
{	
	core::renderer::layer layer;
	auto active = std::make_shared<mock_frame_producer>();
	auto background = std::make_shared<mock_frame_producer>();

	layer.load(active, renderer::load_option::auto_play);
	layer.load(background, renderer::load_option::preview);
		
	auto frame = layer.render_frame();

	ASSERT_TRUE(frame != nullptr);
	ASSERT_TRUE(layer.render_frame() == frame);
	ASSERT_TRUE(layer.active() == nullptr);
	ASSERT_TRUE(layer.background() == background);
}

TEST(layer, load_play)
{	
	core::renderer::layer layer;
	auto active = std::make_shared<mock_frame_producer>();

	layer.load(active);
	layer.play();
	
	ASSERT_TRUE(layer.render_frame() != nullptr);
	ASSERT_TRUE(layer.active() == active);
	ASSERT_TRUE(layer.background() == nullptr);
}


TEST(layer, empty_play)
{	
	core::renderer::layer layer;
	layer.play();

	auto frame = layer.render_frame();

	ASSERT_TRUE(frame == nullptr);
}

TEST(layer, pause)
{
	core::renderer::layer layer;
	auto active = std::make_shared<mock_frame_producer>();

	layer.load(active, renderer::load_option::auto_play);
	
	auto frame = layer.render_frame();

	layer.pause();
	
	ASSERT_TRUE(layer.active() == active);
	ASSERT_TRUE(layer.render_frame() == frame);

	layer.play();
	
	ASSERT_TRUE(layer.active() == active);
	ASSERT_TRUE(layer.render_frame() != frame);
}

TEST(layer, stop)
{
	core::renderer::layer layer;
	auto active = std::make_shared<mock_frame_producer>();
	auto background = std::make_shared<mock_frame_producer>();

	layer.load(active, renderer::load_option::auto_play);
	layer.load(background, renderer::load_option::preview);
	layer.render_frame();

	layer.stop();
		
	ASSERT_TRUE(layer.render_frame() == nullptr);
	ASSERT_TRUE(layer.active() == nullptr);
	ASSERT_TRUE(layer.background() == background);;
}

TEST(layer, clear)
{
	core::renderer::layer layer;
	auto active = std::make_shared<mock_frame_producer>();
	auto background = std::make_shared<mock_frame_producer>();

	layer.load(active, renderer::load_option::auto_play);
	layer.load(background, renderer::load_option::preview);
	layer.clear();
		
	ASSERT_TRUE(layer.render_frame() == nullptr);
	ASSERT_TRUE(layer.active() == nullptr);
	ASSERT_TRUE(layer.background() == nullptr);
}

TEST(layer, following_producer)
{
	core::renderer::layer layer;
	auto active = std::make_shared<mock_frame_producer>(true);
	auto following = std::make_shared<mock_frame_producer>();
	layer.load(active, renderer::load_option::auto_play);
		
	ASSERT_TRUE(layer.render_frame() == nullptr);
	ASSERT_TRUE(layer.active() == nullptr);
		
	layer.load(active, renderer::load_option::auto_play);
	active->set_following_producer(following);
	
	ASSERT_TRUE(layer.render_frame() != nullptr);
	ASSERT_TRUE(layer.active() == following);
}

TEST(layer, leading_producer)
{
	core::renderer::layer layer;
	auto active = std::make_shared<mock_frame_producer>();
	auto active2 = std::make_shared<mock_frame_producer>();
	layer.load(active, renderer::load_option::auto_play);
		
	ASSERT_TRUE(layer.render_frame() != nullptr);
	ASSERT_TRUE(layer.active() == active);
		
	layer.load(active2, renderer::load_option::auto_play);
	
	ASSERT_TRUE(layer.render_frame() != nullptr);
	ASSERT_TRUE(layer.active() == active2);
}

TEST(layer, get_frame_exception)
{
	core::renderer::layer layer;
	auto active = std::make_shared<mock_frame_producer>(false, true);
	layer.load(active, renderer::load_option::auto_play);
			
	ASSERT_TRUE(layer.render_frame() == nullptr);
	ASSERT_TRUE(layer.active() == nullptr);
}