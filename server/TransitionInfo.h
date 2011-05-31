#pragma once

#include <string>

namespace caspar {

enum TransitionType
{
	Cut = 1,
	Mix,
	Push,
	Slide,
	Wipe
};

enum TransitionDirection
{
	FromLeft = 1,
	FromRight,
	FromTop,
	FromBottom
};

class TransitionInfo
{
public:
	TransitionInfo() : type_(Cut), duration_(0), borderWidth_(0), borderColor_(TEXT("#00000000")), direction_(FromLeft)
	{}

	~TransitionInfo()
	{}

	TransitionType		type_;
	unsigned short		duration_;
	unsigned short		borderWidth_;
	tstring			borderImage_;
	tstring			borderColor_;
	TransitionDirection	direction_;
};

}	//namespace caspar