#include "..\..\..\StdAfx.h"

#include "texture_atlas.h"
#include "texture_font.h"

#include <map>
#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H

namespace caspar { namespace core { namespace text {

struct freetype_exception : virtual caspar_exception
{
	freetype_exception() {}
	explicit freetype_exception(const char* msg) : caspar_exception(msg) {}
};

struct unicode_range
{
	int first;
	int last;
};

unicode_range get_range(unicode_block block);


struct texture_font::impl
{
private:
	struct glyph_info
	{
		glyph_info(int w, int h, float l, float t, float r, float b) : width(w), height(h), left(l), top(t), right(r), bottom(b)
		{}

		float left, top, right, bottom;
		int width, height;
	};

	FT_Library		lib_;
	FT_Face			face_;
	texture_atlas	atlas_;
	float			size_;
	std::map<int, glyph_info> glyphs_;

public:
	impl::impl(texture_atlas& atlas, const std::wstring& filename, float size) : lib_(nullptr), face_(nullptr), atlas_(atlas), size_(size)
	{
		try
		{
			FT_Error err;
			err = FT_Init_FreeType(&lib_);
			if(err) throw freetype_exception("Failed to initialize freetype");

			err = FT_New_Face(lib_, u8(filename).c_str(), 0, &face_);
			if(err) throw freetype_exception("Failed to load font");

			err = FT_Set_Char_Size(face_, (FT_F26Dot6)(size*64), 0, 72, 72);
			if(err) throw freetype_exception("Failed to set font size");
		}
		catch(std::exception& ex)
		{
			if(face_ != nullptr)
				FT_Done_Face(face_);
			if(lib_ != nullptr)
				FT_Done_FreeType(lib_);

			throw ex;
		}
	}

	~impl()
	{
		if(face_ != nullptr)
			FT_Done_Face(face_);
		if(lib_ != nullptr)
			FT_Done_FreeType(lib_);
	}

	int count_glyphs_in_range(unicode_block block)
	{ 
		unicode_range range = get_range(block);

		//TODO: extract info from freetype

		//very pesimistic, assumes a glyph for each charcode
		return range.last - range.first;
	}

	void impl::load_glyphs(unicode_block block, const color<float>& col)
	{
		FT_Error err;
		int flags = FT_LOAD_RENDER | FT_LOAD_FORCE_AUTOHINT | FT_LOAD_TARGET_NORMAL;
		unicode_range range = get_range(block);

		for(int i = range.first; i <= range.last; ++i)
		{
			FT_UInt glyph_index = FT_Get_Char_Index(face_, i);
			if(!glyph_index)	//ignore codes that doesn't have a glyph for now. Might want to map these to a special glyph later.
				continue;
			
			err = FT_Load_Glyph(face_, glyph_index, flags);
			if(err) continue;	//igonore glyphs that fail to load

			const FT_Bitmap& bitmap = face_->glyph->bitmap;	//shorthand notation

			auto region = atlas_.get_region(bitmap.width+1, bitmap.rows+1);
			if(region.x < 0)
			{
				//the glyph doesn't fit in the texture-atlas. ignore it for now.
				//we might want to restart with a bigger atlas in the future
				continue;
			}

			atlas_.set_region(region.x, region.y, bitmap.width, bitmap.rows, bitmap.buffer, bitmap.pitch, col);
			glyphs_.insert(std::pair<int, glyph_info>(i, glyph_info(bitmap.width, bitmap.rows, 
										region.x / (float)atlas_.width(), 
										region.y / (float)atlas_.height(), 
										(region.x + bitmap.width) / (float)atlas_.width(), 
										(region.y + bitmap.rows) / (float)atlas_.height())));
		}
	}

	std::vector<float> create_vertex_stream(const std::wstring& str, int x, int y, int parent_width, int parent_height)
	{
		//TODO: detect glyphs that aren't in the atlas and load them (and maybe that entire unicode_block on the fly

		std::vector<float> result(16*str.length(), 0);

		bool use_kerning = (face_->face_flags & FT_FACE_FLAG_KERNING) == FT_FACE_FLAG_KERNING;
		int index = 0;
		FT_UInt previous = 0;
		float pos_x = (float)x;
		float pos_y = (float)y;

		auto end = str.end();
		for(auto it = str.begin(); it != end; ++it, ++index)
		{
			auto glyph_it = glyphs_.find(*it);
			if(glyph_it != glyphs_.end())
			{	
				const glyph_info& coords = glyph_it->second;

				FT_UInt glyph_index = FT_Get_Char_Index(face_, (*it));

				if(use_kerning && previous && glyph_index)
				{
					FT_Vector delta;
					FT_Get_Kerning(face_, previous, glyph_index, FT_KERNING_DEFAULT, &delta);

					pos_x += delta.x / 64.0f;
				}

				FT_Load_Glyph(face_, glyph_index, FT_LOAD_NO_BITMAP | FT_LOAD_FORCE_AUTOHINT | FT_LOAD_TARGET_NORMAL);

				float left = (pos_x + face_->glyph->metrics.horiBearingX/64.0f) / parent_width ;
				float right = ((pos_x + face_->glyph->metrics.horiBearingX/64.0f) + coords.width) / parent_width;

				float top = (pos_y - face_->glyph->metrics.horiBearingY/64.0f) / parent_height;
				float bottom = ((pos_y - face_->glyph->metrics.horiBearingY/64.0f) + coords.height) / parent_height;

				//vertex 1 top left
				result[index*16 + 0] = left;			//vertex.x
				result[index*16 + 1] = top;				//vertex.y
				result[index*16 + 2] = coords.left;		//texcoord.r
				result[index*16 + 3] = coords.top;		//texcoord.s

				//vertex 2 top right
				result[index*16 + 4] = right;			//vertex.x
				result[index*16 + 5] = top;				//vertex.y
				result[index*16 + 6] = coords.right;	//texcoord.r
				result[index*16 + 7] = coords.top;		//texcoord.s

				//vertex 3 bottom right
				result[index*16 + 8] = right;			//vertex.x
				result[index*16 + 9] = bottom;			//vertex.y
				result[index*16 + 10] = coords.right;	//texcoord.r
				result[index*16 + 11] = coords.bottom;	//texcoord.s

				//vertex 4 bottom left
				result[index*16 + 12] = left;			//vertex.x
				result[index*16 + 13] = bottom;			//vertex.y
				result[index*16 + 14] = coords.left;	//texcoord.r
				result[index*16 + 15] = coords.bottom;	//texcoord.s

				pos_x += face_->glyph->advance.x / 64.0f;
				previous = glyph_index;
			}
		}
		return result;
	}

	string_metrics measure_string(const std::wstring& str)
	{
		string_metrics result;
		
		bool use_kerning = (face_->face_flags & FT_FACE_FLAG_KERNING) == FT_FACE_FLAG_KERNING;
		int index = 0;
		FT_UInt previous = 0;
		float pos_x = 0;
//		float pos_y = 0;

		auto end = str.end();
		for(auto it = str.begin(); it != end; ++it, ++index)
		{
			auto glyph_it = glyphs_.find(*it);
			if(glyph_it != glyphs_.end())
			{	
				const glyph_info& coords = glyph_it->second;

				FT_UInt glyph_index = FT_Get_Char_Index(face_, (*it));

				if(use_kerning && previous && glyph_index)
				{
					FT_Vector delta;
					FT_Get_Kerning(face_, previous, glyph_index, FT_KERNING_DEFAULT, &delta);

					pos_x += delta.x / 64.0f;
				}

				FT_Load_Glyph(face_, glyph_index, FT_LOAD_NO_BITMAP | FT_LOAD_FORCE_AUTOHINT | FT_LOAD_TARGET_NORMAL);

				int bearingY = face_->glyph->metrics.horiBearingY >> 6;
				if(bearingY > result.bearingY)
					result.bearingY = bearingY;

				if(coords.height > result.height)
					result.height = coords.height;

				pos_x += face_->glyph->advance.x / 64.0f;
				previous = glyph_index;
			}
		}

		result.width = (int)(pos_x+.5f);
		return result;
	}
}; 

texture_font::texture_font(texture_atlas& atlas, const std::wstring& filename, float size) : impl_(new impl(atlas, filename, size)) {}
void texture_font::load_glyphs(unicode_block range, const color<float>& col) { impl_->load_glyphs(range, col); }
std::vector<float> texture_font::create_vertex_stream(const std::wstring& str, int x, int y, int parent_width, int parent_height) { return impl_->create_vertex_stream(str, x, y, parent_width, parent_height); }
string_metrics texture_font::measure_string(const std::wstring& str) { return impl_->measure_string(str); }

unicode_range get_range(unicode_block block)
{
	//TODO: implement
	unicode_range range = {0x0000, 0x007F};
	return range;
}

}}}

//unicode blocks
/*
		range(0x0000, 0x007F); 
		range(0x0080, 0x00FF); 
		range(0x0100, 0x017F); 
		range(0x0180, 0x024F); 
		range(0x0250, 0x02AF); 
		range(0x02B0, 0x02FF); 
		range(0x0300, 0x036F); 
		range(0x0370, 0x03FF); 
		range(0x0400, 0x04FF); 
		range(0x0500, 0x052F); 
		range(0x0530, 0x058F); 
		range(0x0590, 0x05FF); 
		range(0x0600, 0x06FF); 
		range(0x0700, 0x074F); 
		range(0x0750, 0x077F); 
		range(0x0780, 0x07BF); 
		range(0x07C0, 0x07FF); 
		range(0x0800, 0x083F); 
		range(0x0840, 0x085F); 
		range(0x08A0, 0x08FF); 
		range(0x0900, 0x097F); 
		range(0x0980, 0x09FF); 
		range(0x0A00, 0x0A7F); 
		range(0x0A80, 0x0AFF); 
		range(0x0B00, 0x0B7F); 
		range(0x0B80, 0x0BFF); 
		range(0x0C00, 0x0C7F); 
		range(0x0C80, 0x0CFF); 
		range(0x0D00, 0x0D7F); 
		range(0x0D80, 0x0DFF); 
		range(0x0E00, 0x0E7F); 
		range(0x0E80, 0x0EFF); 
		range(0x0F00, 0x0FFF); 
		range(0x1000, 0x109F); 
		range(0x10A0, 0x10FF); 
		range(0x1100, 0x11FF); 
		range(0x1200, 0x137F); 
		range(0x1380, 0x139F); 
		range(0x13A0, 0x13FF); 
		range(0x1400, 0x167F); 
		range(0x1680, 0x169F); 
		range(0x16A0, 0x16FF); 
		range(0x1700, 0x171F); 
		range(0x1720, 0x173F); 
		range(0x1740, 0x175F); 
		range(0x1760, 0x177F); 
		range(0x1780, 0x17FF); 
		range(0x1800, 0x18AF); 
		range(0x18B0, 0x18FF); 
		range(0x1900, 0x194F); 
		range(0x1950, 0x197F); 
		range(0x1980, 0x19DF); 
		range(0x19E0, 0x19FF); 
		range(0x1A00, 0x1A1F); 
		range(0x1A20, 0x1AAF); 
		range(0x1B00, 0x1B7F); 
		range(0x1B80, 0x1BBF); 
		range(0x1BC0, 0x1BFF); 
		range(0x1C00, 0x1C4F); 
		range(0x1C50, 0x1C7F); 
		range(0x1CC0, 0x1CCF); 
		range(0x1CD0, 0x1CFF); 
		range(0x1D00, 0x1D7F); 
		range(0x1D80, 0x1DBF); 
		range(0x1DC0, 0x1DFF); 
		range(0x1E00, 0x1EFF); 
		range(0x1F00, 0x1FFF); 
		range(0x2000, 0x206F); 
		range(0x2070, 0x209F); 
		range(0x20A0, 0x20CF); 
		range(0x20D0, 0x20FF); 
		range(0x2100, 0x214F); 
		range(0x2150, 0x218F); 
		range(0x2190, 0x21FF); 
		range(0x2200, 0x22FF); 
		range(0x2300, 0x23FF); 
		range(0x2400, 0x243F); 
		range(0x2440, 0x245F); 
		range(0x2460, 0x24FF); 
		range(0x2500, 0x257F); 
		range(0x2580, 0x259F); 
		range(0x25A0, 0x25FF); 
		range(0x2600, 0x26FF); 
		range(0x2700, 0x27BF); 
		range(0x27C0, 0x27EF); 
		range(0x27F0, 0x27FF); 
		range(0x2800, 0x28FF); 
		range(0x2900, 0x297F); 
		range(0x2980, 0x29FF); 
		range(0x2A00, 0x2AFF); 
		range(0x2B00, 0x2BFF); 
		range(0x2C00, 0x2C5F); 
		range(0x2C60, 0x2C7F); 
		range(0x2C80, 0x2CFF); 
		range(0x2D00, 0x2D2F); 
		range(0x2D30, 0x2D7F); 
		range(0x2D80, 0x2DDF); 
		range(0x2DE0, 0x2DFF); 
		range(0x2E00, 0x2E7F); 
		range(0x2E80, 0x2EFF); 
		range(0x2F00, 0x2FDF); 
		range(0x2FF0, 0x2FFF); 
		range(0x3000, 0x303F); 
		range(0x3040, 0x309F); 
		range(0x30A0, 0x30FF); 
		range(0x3100, 0x312F); 
		range(0x3130, 0x318F); 
		range(0x3190, 0x319F); 
		range(0x31A0, 0x31BF); 
		range(0x31C0, 0x31EF); 
		range(0x31F0, 0x31FF); 
		range(0x3200, 0x32FF); 
		range(0x3300, 0x33FF); 
		range(0x3400, 0x4DBF); 
		range(0x4DC0, 0x4DFF); 
		range(0x4E00, 0x9FFF); 
		range(0xA000, 0xA48F); 
		range(0xA490, 0xA4CF); 
		range(0xA4D0, 0xA4FF); 
		range(0xA500, 0xA63F); 
		range(0xA640, 0xA69F); 
		range(0xA6A0, 0xA6FF); 
		range(0xA700, 0xA71F); 
		range(0xA720, 0xA7FF); 
		range(0xA800, 0xA82F); 
		range(0xA830, 0xA83F); 
		range(0xA840, 0xA87F); 
		range(0xA880, 0xA8DF); 
		range(0xA8E0, 0xA8FF); 
		range(0xA900, 0xA92F); 
		range(0xA930, 0xA95F); 
		range(0xA960, 0xA97F); 
		range(0xA980, 0xA9DF); 
		range(0xAA00, 0xAA5F); 
		range(0xAA60, 0xAA7F); 
		range(0xAA80, 0xAADF); 
		range(0xAAE0, 0xAAFF); 
		range(0xAB00, 0xAB2F); 
		range(0xABC0, 0xABFF); 
		range(0xAC00, 0xD7AF); 
		range(0xD7B0, 0xD7FF); 
		range(0xD800, 0xDB7F); 
		range(0xDB80, 0xDBFF); 
		range(0xDC00, 0xDFFF); 
		range(0xE000, 0xF8FF); 
		range(0xF900, 0xFAFF); 
		range(0xFB00, 0xFB4F); 
		range(0xFB50, 0xFDFF); 
		range(0xFE00, 0xFE0F); 
		range(0xFE10, 0xFE1F); 
		range(0xFE20, 0xFE2F); 
		range(0xFE30, 0xFE4F); 
		range(0xFE50, 0xFE6F); 
		range(0xFE70, 0xFEFF); 
		range(0xFF00, 0xFFEF); 
		range(0xFFF0, 0xFFFF); 
		range(0x10000, 0x1007F); 
		range(0x10080, 0x100FF); 
		range(0x10100, 0x1013F); 
		range(0x10140, 0x1018F); 
		range(0x10190, 0x101CF); 
		range(0x101D0, 0x101FF); 
		range(0x10280, 0x1029F); 
		range(0x102A0, 0x102DF); 
		range(0x10300, 0x1032F); 
		range(0x10330, 0x1034F); 
		range(0x10380, 0x1039F); 
		range(0x103A0, 0x103DF); 
		range(0x10400, 0x1044F); 
		range(0x10450, 0x1047F); 
		range(0x10480, 0x104AF); 
		range(0x10800, 0x1083F); 
		range(0x10840, 0x1085F); 
		range(0x10900, 0x1091F); 
		range(0x10920, 0x1093F); 
		range(0x10980, 0x1099F); 
		range(0x109A0, 0x109FF); 
		range(0x10A00, 0x10A5F); 
		range(0x10A60, 0x10A7F); 
		range(0x10B00, 0x10B3F); 
		range(0x10B40, 0x10B5F); 
		range(0x10B60, 0x10B7F); 
		range(0x10C00, 0x10C4F); 
		range(0x10E60, 0x10E7F); 
		range(0x11000, 0x1107F); 
		range(0x11080, 0x110CF); 
		range(0x110D0, 0x110FF); 
		range(0x11100, 0x1114F); 
		range(0x11180, 0x111DF); 
		range(0x11680, 0x116CF); 
		range(0x12000, 0x123FF); 
		range(0x12400, 0x1247F); 
		range(0x13000, 0x1342F); 
		range(0x16800, 0x16A3F); 
		range(0x16F00, 0x16F9F); 
		range(0x1B000, 0x1B0FF); 
		range(0x1D000, 0x1D0FF); 
		range(0x1D100, 0x1D1FF); 
		range(0x1D200, 0x1D24F); 
		range(0x1D300, 0x1D35F); 
		range(0x1D360, 0x1D37F); 
		range(0x1D400, 0x1D7FF); 
		range(0x1EE00, 0x1EEFF); 
		range(0x1F000, 0x1F02F); 
		range(0x1F030, 0x1F09F); 
		range(0x1F0A0, 0x1F0FF); 
		range(0x1F100, 0x1F1FF); 
		range(0x1F200, 0x1F2FF); 
		range(0x1F300, 0x1F5FF); 
		range(0x1F600, 0x1F64F); 
		range(0x1F680, 0x1F6FF); 
		range(0x1F700, 0x1F77F); 
		range(0x20000, 0x2A6DF); 
		range(0x2A700, 0x2B73F); 
		range(0x2B740, 0x2B81F); 
		range(0x2F800, 0x2FA1F); 
		range(0xE0000, 0xE007F); 
		range(0xE0100, 0xE01EF); 
		range(0xF0000, 0xFFFFF); 
		range(0x100000, 0x10FFFF);
*/