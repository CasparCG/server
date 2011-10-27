#pragma once

namespace caspar { namespace ffmpeg {
	
std::map<std::string, std::string> read_flv_meta_info(const std::string& filename);

}}