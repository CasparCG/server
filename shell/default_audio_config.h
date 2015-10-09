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
* Author: Helge Norberg, helge.norberg@svt.se
*/

#pragma once

#include <string>

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>

namespace caspar {

std::wstring get_default_audio_config_xml()
{
	return LR"(
		<audio>
			<channel-layouts>
				<channel-layout name="mono"        type="mono"        num-channels="1" channel-order="FC" />
				<channel-layout name="stereo"      type="stereo"      num-channels="2" channel-order="FL FR" />
				<channel-layout name="matrix"      type="matrix"      num-channels="2" channel-order="ML MR" />
				<channel-layout name="film"        type="5.1"         num-channels="6" channel-order="FL FC FR BL BR LFE" />
				<channel-layout name="smpte"       type="5.1"         num-channels="6" channel-order="FL FR FC LFE BL BR" />
				<channel-layout name="ebu_r123_8a" type="5.1+downmix" num-channels="8" channel-order="DL DR FL FR FC LFE BL BR" />
				<channel-layout name="ebu_r123_8b" type="5.1+downmix" num-channels="8" channel-order="FL FR FC LFE BL BR DL DR" />
				<channel-layout name="8ch"         type="8ch"         num-channels="8" />
				<channel-layout name="16ch"        type="16ch"        num-channels="16" />
			</channel-layouts>
			<mix-configs>
				<mix-config from-type="mono"          to-types="stereo, 5.1"  mix="FL = FC                                           | FR = FC" />
				<mix-config from-type="mono"          to-types="5.1+downmix"  mix="FL = FC                                           | FR = FC                                         | DL = FC | DR = FC" />
				<mix-config from-type="mono"          to-types="matrix"       mix="ML = FC                                           | MR = FC" />
				<mix-config from-type="stereo"        to-types="mono"         mix="FC &lt; FL + FR" />
				<mix-config from-type="stereo"        to-types="matrix"       mix="ML = FL                                           | MR = FR" />
				<mix-config from-type="stereo"        to-types="5.1"          mix="FL = FL                                           | FR = FR" />
				<mix-config from-type="stereo"        to-types="5.1+downmix"  mix="FL = FL                                           | FR = FR                                         | DL = FL | DR = FR" />
				<mix-config from-type="5.1"           to-types="mono"         mix="FC &lt; FL + FR + 0.707*FC + 0.707*BL + 0.707*BR" />
				<mix-config from-type="5.1"           to-types="stereo"       mix="FL &lt; FL + 0.707*FC + 0.707*BL                  | FR &lt; FR + 0.707*FC + 0.707*BR" />
				<mix-config from-type="5.1"           to-types="5.1+downmix"  mix="FL = FL                                           | FR = FR                                         | FC = FC | BL = BL | BR = BR | LFE = LFE | DL &lt; FL + 0.707*FC + 0.707*BL | DR &lt; FR + 0.707*FC + 0.707*BR" />
				<mix-config from-type="5.1"           to-types="matrix"       mix="ML = 0.3204*FL + 0.293*FC + -0.293*BL + -0.293*BR | MR = 0.3204*FR + 0.293*FC + 0.293*BL + 0.293*BR" />
				<mix-config from-type="5.1+stereomix" to-types="mono"         mix="FC &lt; DL + DR" />
				<mix-config from-type="5.1+stereomix" to-types="stereo"       mix="FL = DL                                           | FR = DR" />
				<mix-config from-type="5.1+stereomix" to-types="5.1"          mix="FL = FL                                           | FR = FR                                         | FC = FC | BL = BL | BR = BR | LFE = LFE" />
				<mix-config from-type="5.1+stereomix" to-types="matrix"       mix="ML = 0.3204*FL + 0.293*FC + -0.293*BL + -0.293*BR | MR = 0.3204*FR + 0.293*FC + 0.293*BL + 0.293*BR" />
			</mix-configs>
		</audio>
	)";
}

boost::property_tree::wptree get_default_audio_config()
{
	std::wstringstream stream(get_default_audio_config_xml());
	boost::property_tree::wptree result;
	boost::property_tree::xml_parser::read_xml(stream, result, boost::property_tree::xml_parser::trim_whitespace | boost::property_tree::xml_parser::no_comments);

	return result;
}

}
