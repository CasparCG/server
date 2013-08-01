// psd-test.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "../../modules/psd/doc.h"
#include "../../modules/psd/layer.h"
#include "../../common/utf.h"

#include <sstream>
#include <iostream>

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/detail/file_parser_error.hpp>
#include <boost/property_tree/xml_parser.hpp>


int _tmain(int argc, _TCHAR* argv[])
{
	caspar::psd::psd_document doc;
	doc.parse(L"C:\\Lokala Filer\\Utveckling\\CasparCG\\Server 2.1\\shell\\media\\test1.psd");

	std::wstringstream trace;

	trace << L"<doc filename='" << doc.filename() << L"' color_mode='" << caspar::psd::color_mode_to_string(doc.color_mode()) << L"' color_depth='" << doc.color_depth() << L"' channel_count='" << doc.channels_count() << L"' width='" << doc.width() << L"' height='" << doc.height() << L"'>" << std::endl;
	
	auto end = doc.layers().end();
	for(auto it = doc.layers().begin(); it != end; ++it)
	{
		caspar::psd::layer_ptr layer = (*it);
		trace << L"	<layer name='" << layer->name() << L"' opacity='" << layer->opacity() << L"' flags='" << std::hex << layer->flags() << std::dec << "'>" << std::endl;
		if(layer->image())
			trace << L"		<bounding-box left='" << layer->rect().left << "' top='" << layer->rect().top << "' right='" << layer->rect().right << "' bottom='" << layer->rect().bottom << "' />" << std::endl;
		if(layer->mask())
			trace << L"		<mask default-value='" << layer->mask_info().default_value_ << "' flags='" <<  std::hex << (unsigned short) layer->mask_info().flags_ << std::dec << "' left='" << layer->mask_info().rect_.left << "' top='" << layer->mask_info().rect_.top << "' right='" << layer->mask_info().rect_.right << "' bottom='" << layer->mask_info().rect_.bottom << "' />" << std::endl;
		if(layer->is_text())
		{
			boost::property_tree::xml_writer_settings<wchar_t> w(' ', 3);
			boost::property_tree::write_xml(trace, (*it)->text_data(), w);
		}
		trace << L"	</layer>" << std::endl;
	}

	trace << L"</doc>" << std::endl;
	
	std::ofstream log("psd-log.txt");
	log << caspar::u8(trace.str());
	std::cout << caspar::u8(trace.str());
	
	return 0;
}

