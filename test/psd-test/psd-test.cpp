// psd-test.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "../../modules/psd/doc.h"

int _tmain(int argc, _TCHAR* argv[])
{
	caspar::psd::Document doc;
	doc.parse(L"C:\\Lokala Filer\\Utveckling\\CasparCG\\Server 2.1\\test\\data\\test1.psd");
	int a = 42;
	return 0;
}

