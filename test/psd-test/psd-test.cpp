// psd-test.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "../../modules/psd/doc.h"
#include <sstream>

int _tmain(int argc, _TCHAR* argv[])
{
	caspar::psd::psd_document doc;
	doc.parse(L"C:\\Lokala Filer\\Utveckling\\CasparCG\\Server 2.1\\test\\data\\test1.psd");

	std::wstringstream trace;

	trace << L"<doc filename=\"" << doc.filename() << L"\" channel_count=\"" << doc.channels_count() << L"\" width=\"" << doc.width() << "\" 
	int a = 42;
	return 0;
}

