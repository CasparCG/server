#include <gtest/gtest.h>
#include <conio.h>
#include <windows.h>

int main(int argc, char **argv)
{
	MoveWindow(GetConsoleWindow(), 0, 0, 800, 1000, true);
	::testing::InitGoogleTest(&argc, argv);
	RUN_ALL_TESTS();
	getchar();
}