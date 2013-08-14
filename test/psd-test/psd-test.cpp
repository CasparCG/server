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
	if(argc > 1)
	{
		caspar::psd::psd_document doc;
		if(doc.parse(argv[1]))
		{
			std::wstringstream trace;

			trace << L"<doc filename='" << doc.filename() << L"' color_mode='" << caspar::psd::color_mode_to_string(doc.color_mode()) << L"' color_depth='" << doc.color_depth() << L"' channel_count='" << doc.channels_count() << L"' width='" << doc.width() << L"' height='" << doc.height() << L"'>" << std::endl;
			if(doc.has_timeline())
			{
				trace << L"<timeline>" << std::endl;
					boost::property_tree::xml_writer_settings<wchar_t> w(' ', 3);
					boost::property_tree::write_xml(trace, doc.timeline(), w);

				trace << L"</timeline>" << std::endl;

			}
	
			auto end = doc.layers().end();
			for(auto it = doc.layers().begin(); it != end; ++it)
			{
				caspar::psd::layer_ptr layer = (*it);
				trace << L"	<layer name='" << layer->name() << L"' opacity='" << layer->opacity() << L"' visible='" << layer->visible() << L"' protection-flags='" << std::hex << layer->protection_flags() << std::dec << L"'>" << std::endl;
				if(layer->image())
					trace << L"		<bounding-box left='" << layer->rect().left << L"' top='" << layer->rect().top << L"' right='" << layer->rect().right << L"' bottom='" << layer->rect().bottom << L"' />" << std::endl;
				if(layer->mask())
					trace << L"		<mask default-value='" << layer->mask_info().default_value_ << L"' enabled=" << layer->mask_info().enabled() << L" linked=" << layer->mask_info().linked() << L" inverted=" << layer->mask_info().inverted() << L" left='" << layer->mask_info().rect_.left << L"' top='" << layer->mask_info().rect_.top << "' right='" << layer->mask_info().rect_.right << "' bottom='" << layer->mask_info().rect_.bottom << "' />" << std::endl;
				if(layer->is_text())
				{
					trace << L"			<text value='" << (*it)->text_data().get(L"EngineDict.Editor.Text", L"") << L"' />" << std::endl;
					//boost::property_tree::xml_writer_settings<wchar_t> w(' ', 3);
					//boost::property_tree::write_xml(trace, (*it)->text_data(), w);
				}
				if(layer->has_timeline())
				{
					trace << L"			<timeline>" << std::endl;
					boost::property_tree::xml_writer_settings<wchar_t> w(' ', 3);
					boost::property_tree::write_xml(trace, (*it)->timeline_data(), w);
					trace << L"			</timeline>" << std::endl;
				}
				trace << L"	</layer>" << std::endl;
			}

			trace << L"</doc>" << std::endl;
	
			std::ofstream log("psd-log.txt");
			log << caspar::u8(trace.str());
			std::cout << caspar::u8(trace.str());
		}
	}
	return 0;
}

