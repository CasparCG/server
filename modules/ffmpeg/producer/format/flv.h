#pragma once

namespace caspar {
	   
struct flv_meta_info
{
   double duration;
   double width;
   double height;
   double frame_rate;
   double video_data_rate;
   double audio_data_rate;

   flv_meta_info()
	   : duration(0.0)
	   , width(0.0)
	   , height(0.0)
	   , frame_rate(0.0)
	   , video_data_rate(0.0)
	   , audio_data_rate(0.0)
   {}
};

flv_meta_info read_flv_meta_info(const std::wstring& filename);

}