/*
* Copyright (c) 2011 Sveriges Television AB <info@casparcg.com>
*
* This file is part of CasparCG (www.casparcg.com).
*
* CasparCG is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* CasparCG is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with CasparCG. If not, see <http://www.gnu.org/licenses/>.
*
* Author: Niklas P Andersson, niklas.p.andersson@svt.se
*/

#include "psd_document.h"
#include "descriptor.h"
#include <iostream>

#include <common/log.h>

namespace caspar { namespace psd {

psd_document::psd_document()
{
}

void psd_document::parse(const std::wstring& filename)
{
	// Most of the parsing here is based on information from http://www.adobe.com/devnet-apps/photoshop/fileformatashtml/
	filename_ = filename;
	input_.open(filename_);
	read_header();
	read_color_mode();
	read_image_resources();
	read_layers();
}

void psd_document::read_header()
{
	auto signature = input_.read_long();
	auto version = input_.read_short();
	if(!(signature == '8BPS' && version == 1))
		CASPAR_THROW_EXCEPTION(psd_file_format_exception() << msg_info("!(signature == '8BPS' && version == 1)"));

	input_.discard_bytes(6);
	channels_= input_.read_short();
	height_ = input_.read_long();
	width_ = input_.read_long();
	depth_ = input_.read_short();	//bits / channel

	color_mode_ = int_to_color_mode(input_.read_short());
}

void psd_document::read_color_mode()
{
	auto length = input_.read_long();
	input_.discard_bytes(length);
}

void psd_document::read_image_resources()
{
	auto section_length = input_.read_long();

	if(section_length > 0)
	{
		std::streamoff end_of_section = input_.current_position() + section_length;
	
		try
		{
			while(input_.current_position() < end_of_section)
			{
				auto signature = input_.read_long();
				if(signature != '8BIM')
					CASPAR_THROW_EXCEPTION(psd_file_format_exception() << msg_info("signature != '8BIM'"));

				auto resource_id = input_.read_short();
				
				std::wstring name = input_.read_pascal_string(2);

				auto resource_length = input_.read_long();
				auto end_of_chunk = input_.current_position() + resource_length + resource_length%2;	//the actual data is padded to even bytes

				try
				{
					//TODO: read actual data
					switch(resource_id)
					{
					case 1026:		//layer group information. describes linked layers
						{
							int layer_count = resource_length / 2;
							for(int i = 0; i < layer_count; ++i)
							{
								int id = static_cast<std::int16_t>(input_.read_short());

								if(i == layers_.size())
									layers_.push_back(std::make_shared<layer>());

								layers_[i]->set_link_group_id(id);
							}

						}
						break;
					case 1075:		//timeline information
						{
							input_.read_long();	//descriptor version, should be 16
							descriptor timeline_descriptor(L"global timeline");
							timeline_descriptor.populate(input_);

							if (timeline_desc_.empty())
								timeline_desc_.swap(timeline_descriptor.items());
						}
						break;
					case 1072:		//layer group(s) enabled id
						{

						}

					case 1005:
						{	
							////resolutionInfo
							//struct ResolutionInfo
							//{
							//Fixed hRes;
							//int16 hResUnit;
							//int16 widthUnit;
							//Fixed vRes;
							//int16 vResUnit;
							//int16 heightUnit;
							//};
						}
					case 1006:		//names of alpha channels
					case 1008:		//caption
					case 1010:		//background color
					case 1024:		//layer state info (2 bytes containing the index of target layer (0 = bottom layer))
					case 1028:		//IPTC-NAA. File Info...
					case 1029:		//image for raw format files
					case 1036:		//thumbnail resource
					case 1045:		//unicode Alpha names (Unicode string (4 bytes length followed by string))
					case 1053:		//alpha identifiers (4 bytes of length, followed by 4 bytes each for every alpha identifier.)
					case 1060:		//XMP metadata
					case 1065:		//layer comps
					case 1069:		//layer selection ID(s)
					case 1077:		//DisplayInfo
					case 2999:		//name of clipping path
					default:
						{
							if(resource_id >= 2000 && resource_id <=2997)	//path information
							{
							}
							else if(resource_id >= 4000 && resource_id <= 4999)	//plug-in resources
							{
							}
						}
						input_.discard_bytes(resource_length);
						break;
					}

					if((resource_length & 1) == 1)	//each resource is padded to an even amount of bytes
						input_.discard_bytes(1);
				}
				catch(psd_file_format_exception&)
				{
					input_.set_position(end_of_chunk);
				}
			}
		}
		catch(psd_file_format_exception&)
		{
			//if an error occurs, just skip this section
			input_.set_position(end_of_section);
		}
	}
}


void psd_document::read_layers()
{
	//"Layer And Mask information"
	auto total_length = input_.read_long();	//length of "Layer and Mask information"
	auto end_of_layers = input_.current_position() + total_length;

	try
	{
		//"Layer info section"
		{
			auto layer_info_length = input_.read_long();	//length of "Layer info" section
			auto end_of_layers_info = input_.current_position() + layer_info_length;

			int layers_count = static_cast<std::int16_t>(input_.read_short());
			//bool has_merged_alpha = (layers_count < 0);
			layers_count = std::abs(layers_count);
			//std::clog << "Expecting " << layers_count << " layers" << std::endl;

			for(int layer_index = 0; layer_index < layers_count; ++layer_index)
			{
				if(layer_index  == layers_.size())
					layers_.push_back(std::make_shared<layer>());
				
				layers_[layer_index]->populate(input_, *this);
				//std::clog << "Added layer: " << std::string(layers_[layerIndex]->name().begin(), layers_[layerIndex]->name().end()) << std::endl;
			}

			auto end = layers_.end();
			for(auto layer_it = layers_.begin(); layer_it != end; ++layer_it)
			{
				(*layer_it)->read_channel_data(input_);	//each layer reads it's "image data"
			}

			input_.set_position(end_of_layers_info);
		}

		//global layer mask info
		auto global_layer_mask_length = input_.read_long();
		input_.discard_bytes(global_layer_mask_length);

	}
	catch(std::exception&)
	{
		CASPAR_LOG_CURRENT_EXCEPTION();
		input_.set_position(end_of_layers);
	}
}

}	//namespace psd
}	//namespace caspar
