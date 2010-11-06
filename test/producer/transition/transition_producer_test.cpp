#include <gtest/gtest.h>

#include "../../mock/mock_frame_producer.h"
#include "../../mock/mock_frame.h"

#include <core/frame/frame_format.h>
#include <core/frame/gpu_frame.h>
#include <core/producer/transition/transition_producer.h>
#include <common/exception/exceptions.h>

using namespace caspar;
using namespace caspar::core;

frame_format_desc test_format = frame_format_desc::format_descs[frame_format::pal];

TEST(transition_producer, constructor_dest_nullptr) 
{
	ASSERT_THROW(transition_producer(nullptr, transition_info(), test_format);
					, null_argument);	
}

TEST(transition_producer, get_following_producer) 
{
	auto dest = std::make_shared<mock_frame_producer>();
	
	transition_producer producer(dest, transition_info(), test_format);

	ASSERT_TRUE(producer.get_following_producer() == dest);
}

TEST(transition_producer, get_frame_format_desc) 
{
	auto dest = std::make_shared<mock_frame_producer>();
	
	transition_producer producer(dest, transition_info(), test_format);

	ASSERT_TRUE(producer.get_frame_format_desc() == test_format);
}

TEST(transition_producer, null_dest_get_frame) 
{
	auto source = std::make_shared<mock_frame_producer>();
	
	transition_producer producer(nullptr, transition_info(), test_format);
	producer.set_leading_producer(source);

	ASSERT_TRUE(producer.get_frame() == nullptr);
}

TEST(transition_producer, null_source_get_frame_cut) 
{
	auto dest = std::make_shared<mock_frame_producer>();
	
	transition_info info;
	info.type = transition_type::cut;

	transition_producer producer(dest, info, test_format);

	ASSERT_TRUE(producer.get_frame() == nullptr);
}

TEST(transition_producer, initialize)
{
	auto dest = std::make_shared<mock_frame_producer>();
	
	transition_producer producer(dest, transition_info(), test_format);
	producer.initialize(nullptr);
	
	ASSERT_TRUE(dest->is_initialized());
}

TEST(transition_producer, duration) 
{
	auto dest = std::make_shared<mock_frame_producer>();
	auto source = std::make_shared<mock_frame_producer>();
	
	transition_info info;
	info.type = transition_type::cut;
	info.duration = 3;

	transition_producer producer(dest, info, test_format);
	producer.set_leading_producer(source);

	for(int n = 0; n < info.duration; ++n)
	{
		auto frame = producer.get_frame();
		ASSERT_TRUE(std::static_pointer_cast<mock_frame>(frame)->tag == source.get());
	}
	ASSERT_TRUE(producer.get_frame() == nullptr);
}