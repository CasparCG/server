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
	unicode_range() : first(0), last(0) {}
	unicode_range(int f, int l) : first(f), last(l) {}

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

	std::shared_ptr<FT_LibraryRec_>	lib_;
	std::shared_ptr<FT_FaceRec_>	face_;
	texture_atlas					atlas_;
	float							size_;
	float							tracking_;
	bool							normalize_;
	std::map<int, glyph_info>		glyphs_;

public:
	impl(texture_atlas& atlas, const text_info& info, bool normalize_coordinates) : atlas_(atlas), size_(info.size), tracking_(info.size*info.tracking/1000.0f), normalize_(normalize_coordinates)
	{
		FT_Library lib;
			
		if (FT_Init_FreeType(&lib))
			throw freetype_exception("Failed to initialize freetype");

		lib_.reset(lib, [](FT_Library ptr) { FT_Done_FreeType(ptr); });

		FT_Face face;
			
		if (FT_New_Face(lib_.get(), u8(info.font_file).c_str(), 0, &face))
			throw freetype_exception("Failed to load font");

		face_.reset(face, [](FT_Face ptr) { FT_Done_Face(ptr); });

		if (FT_Set_Char_Size(face_.get(), static_cast<FT_F26Dot6>(size_*64), 0, 72, 72))
			throw freetype_exception("Failed to set font size");
	}

	void set_tracking(int tracking)
	{
		tracking_ = size_ * tracking / 1000.0f;
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
			FT_UInt glyph_index = FT_Get_Char_Index(face_.get(), i);
			if(!glyph_index)	//ignore codes that doesn't have a glyph for now. Might want to map these to a special glyph later.
				continue;
			
			err = FT_Load_Glyph(face_.get(), glyph_index, flags);
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

	std::vector<float> create_vertex_stream(const std::wstring& str, int x, int y, int parent_width, int parent_height, string_metrics* metrics)
	{
		//TODO: detect glyphs that aren't in the atlas and load them (and maybe that entire unicode_block on the fly

		std::vector<float> result(16*str.length(), 0);

		bool use_kerning = (face_->face_flags & FT_FACE_FLAG_KERNING) == FT_FACE_FLAG_KERNING;
		int index = 0;
		FT_UInt previous = 0;
		float pos_x = (float)x;
		float pos_y = (float)y;

		int maxBearingY = 0;
		int maxProtrudeUnderY = 0;
		int maxHeight = 0;

		auto end = str.end();
		for(auto it = str.begin(); it != end; ++it, ++index)
		{
			auto glyph_it = glyphs_.find(*it);
			if(glyph_it != glyphs_.end())
			{	
				const glyph_info& coords = glyph_it->second;

				FT_UInt glyph_index = FT_Get_Char_Index(face_.get(), (*it));

				if(use_kerning && previous && glyph_index)
				{
					FT_Vector delta;
					FT_Get_Kerning(face_.get(), previous, glyph_index, FT_KERNING_DEFAULT, &delta);

					pos_x += delta.x / 64.0f;
				}

				FT_Load_Glyph(face_.get(), glyph_index, FT_LOAD_NO_BITMAP | FT_LOAD_FORCE_AUTOHINT | FT_LOAD_TARGET_NORMAL);

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

				int bearingY = face_->glyph->metrics.horiBearingY >> 6;

				if(bearingY > maxBearingY)
					maxBearingY = bearingY;

				int protrudeUnderY = coords.height - bearingY;

				if (protrudeUnderY > maxProtrudeUnderY)
					maxProtrudeUnderY = protrudeUnderY;

				if (maxBearingY + maxProtrudeUnderY > maxHeight)
					maxHeight = maxBearingY + maxProtrudeUnderY;

				pos_x += face_->glyph->advance.x / 64.0f;
				pos_x += tracking_;
				previous = glyph_index;
			}
			else
			{
				//TODO: maybe we should try to load the glyph on the fly if it is missing.
			}
		}

		if(normalize_)
		{
			float ratio_x = parent_width/(pos_x - x);
			float ratio_y = parent_height/(float)(maxHeight);
			for(index = 0; index < result.size(); index += 4)
			{
				result[index + 0] *= ratio_x;
				result[index + 1] *= ratio_y;
			}
		}

		if(metrics != nullptr)
		{
			metrics->width = (int)(pos_x - x + 0.5f);
			metrics->bearingY = maxBearingY;
			metrics->height = maxHeight;
			metrics->protrudeUnderY = maxProtrudeUnderY;
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

				FT_UInt glyph_index = FT_Get_Char_Index(face_.get(), (*it));

				if(use_kerning && previous && glyph_index)
				{
					FT_Vector delta;
					FT_Get_Kerning(face_.get(), previous, glyph_index, FT_KERNING_DEFAULT, &delta);

					pos_x += delta.x / 64.0f;
				}

				FT_Load_Glyph(face_.get(), glyph_index, FT_LOAD_NO_BITMAP | FT_LOAD_FORCE_AUTOHINT | FT_LOAD_TARGET_NORMAL);

				int bearingY = face_->glyph->metrics.horiBearingY >> 6;
				if(bearingY > result.bearingY)
					result.bearingY = bearingY;

				int protrudeUnderY = coords.height - bearingY;

				if (protrudeUnderY > result.protrudeUnderY)
					result.protrudeUnderY = protrudeUnderY;

				if (result.bearingY + result.protrudeUnderY > result.height)
					 result.height = result.bearingY + result.protrudeUnderY;

				pos_x += face_->glyph->advance.x / 64.0f;
				previous = glyph_index;
			}
		}

		result.width = (int)(pos_x+.5f);
		return result;
	}
}; 

texture_font::texture_font(texture_atlas& atlas, const text_info& info, bool normalize_coordinates) : impl_(new impl(atlas, info, normalize_coordinates)) {}
void texture_font::load_glyphs(unicode_block range, const color<float>& col) { impl_->load_glyphs(range, col); }
void texture_font::set_tracking(int tracking) { impl_->set_tracking(tracking); }
std::vector<float> texture_font::create_vertex_stream(const std::wstring& str, int x, int y, int parent_width, int parent_height, string_metrics* metrics) { return impl_->create_vertex_stream(str, x, y, parent_width, parent_height, metrics); }
string_metrics texture_font::measure_string(const std::wstring& str) { return impl_->measure_string(str); }

unicode_range get_range(unicode_block block)
{
	switch(block)
	{
		case unicode_block::Basic_Latin: return unicode_range(0x0000, 0x007F);
		case unicode_block::Latin_1_Supplement: return unicode_range(0x0080, 0x00FF);
		case unicode_block::Latin_Extended_A: return		unicode_range(0x0100, 0x017F);
		case unicode_block::Latin_Extended_B: return		unicode_range(0x0180, 0x024F);
		case unicode_block::IPA_Extensions: return		unicode_range(0x0250, 0x02AF);
		case unicode_block::Spacing_Modifier_Letters: return		unicode_range(0x02B0, 0x02FF);
		case unicode_block::Combining_Diacritical_Marks: return		unicode_range(0x0300, 0x036F);
		case unicode_block::Greek_and_Coptic: return		unicode_range(0x0370, 0x03FF);
		case unicode_block::Cyrillic: return		unicode_range(0x0400, 0x04FF);
		case unicode_block::Cyrillic_Supplement: return		unicode_range(0x0500, 0x052F);
		case unicode_block::Armenian: return		unicode_range(0x0530, 0x058F);
		case unicode_block::Hebrew: return		unicode_range(0x0590, 0x05FF);
		case unicode_block::Arabic: return		unicode_range(0x0600, 0x06FF);
		case unicode_block::Syriac: return		unicode_range(0x0700, 0x074F);
		case unicode_block::Arabic_Supplement: return		unicode_range(0x0750, 0x077F);
		case unicode_block::Thaana: return		unicode_range(0x0780, 0x07BF);
		case unicode_block::NKo: return		unicode_range(0x07C0, 0x07FF);
		case unicode_block::Samaritan: return		unicode_range(0x0800, 0x083F);
		case unicode_block::Mandaic: return		unicode_range(0x0840, 0x085F);
		case unicode_block::Arabic_Extended_A: return		unicode_range(0x08A0, 0x08FF);
		case unicode_block::Devanagari: return		unicode_range(0x0900, 0x097F);
		case unicode_block::Bengali: return		unicode_range(0x0980, 0x09FF);
		case unicode_block::Gurmukhi: return		unicode_range(0x0A00, 0x0A7F);
		case unicode_block::Gujarati: return		unicode_range(0x0A80, 0x0AFF);
		case unicode_block::Oriya: return		unicode_range(0x0B00, 0x0B7F);
		case unicode_block::Tamil: return		unicode_range(0x0B80, 0x0BFF);
		case unicode_block::Telugu: return		unicode_range(0x0C00, 0x0C7F);
		case unicode_block::Kannada: return		unicode_range(0x0C80, 0x0CFF);
		case unicode_block::Malayalam: return		unicode_range(0x0D00, 0x0D7F);
		case unicode_block::Sinhala: return		unicode_range(0x0D80, 0x0DFF);
		case unicode_block::Thai: return		unicode_range(0x0E00, 0x0E7F);
		case unicode_block::Lao: return		unicode_range(0x0E80, 0x0EFF);
		case unicode_block::Tibetan: return		unicode_range(0x0F00, 0x0FFF);
		case unicode_block::Myanmar: return		unicode_range(0x1000, 0x109F);
		case unicode_block::Georgian: return		unicode_range(0x10A0, 0x10FF);
		case unicode_block::Hangul_Jamo: return		unicode_range(0x1100, 0x11FF);
		case unicode_block::Ethiopic: return		unicode_range(0x1200, 0x137F);
		case unicode_block::Ethiopic_Supplement: return		unicode_range(0x1380, 0x139F);
		case unicode_block::Cherokee: return		unicode_range(0x13A0, 0x13FF);
		case unicode_block::Unified_Canadian_Aboriginal_Syllabics: return		unicode_range(0x1400, 0x167F);
		case unicode_block::Ogham: return		unicode_range(0x1680, 0x169F);
		case unicode_block::Runic: return		unicode_range(0x16A0, 0x16FF);
		case unicode_block::Tagalog: return		unicode_range(0x1700, 0x171F);
		case unicode_block::Hanunoo: return		unicode_range(0x1720, 0x173F);
		case unicode_block::Buhid: return		unicode_range(0x1740, 0x175F);
		case unicode_block::Tagbanwa: return		unicode_range(0x1760, 0x177F);
		case unicode_block::Khmer: return		unicode_range(0x1780, 0x17FF);
		case unicode_block::Mongolian: return		unicode_range(0x1800, 0x18AF);
		case unicode_block::Unified_Canadian_Aboriginal_Syllabics_Extended: return		unicode_range(0x18B0, 0x18FF);
		case unicode_block::Limbu: return		unicode_range(0x1900, 0x194F);
		case unicode_block::Tai_Le: return		unicode_range(0x1950, 0x197F);
		case unicode_block::New_Tai_Lue: return		unicode_range(0x1980, 0x19DF);
		case unicode_block::Khmer_Symbols: return		unicode_range(0x19E0, 0x19FF);
		case unicode_block::Buginese: return		unicode_range(0x1A00, 0x1A1F);
		case unicode_block::Tai_Tham: return		unicode_range(0x1A20, 0x1AAF);
		case unicode_block::Balinese: return		unicode_range(0x1B00, 0x1B7F);
		case unicode_block::Sundanese: return		unicode_range(0x1B80, 0x1BBF);
		case unicode_block::Batak: return		unicode_range(0x1BC0, 0x1BFF);
		case unicode_block::Lepcha: return		unicode_range(0x1C00, 0x1C4F);
		case unicode_block::Ol_Chiki: return		unicode_range(0x1C50, 0x1C7F);
		case unicode_block::Sundanese_Supplement: return		unicode_range(0x1CC0, 0x1CCF);
		case unicode_block::Vedic_Extensions: return		unicode_range(0x1CD0, 0x1CFF);
		case unicode_block::Phonetic_Extensions: return		unicode_range(0x1D00, 0x1D7F);
		case unicode_block::Phonetic_Extensions_Supplement: return		unicode_range(0x1D80, 0x1DBF);
		case unicode_block::Combining_Diacritical_Marks_Supplement: return		unicode_range(0x1DC0, 0x1DFF);
		case unicode_block::Latin_Extended_Additional: return		unicode_range(0x1E00, 0x1EFF);
		case unicode_block::Greek_Extended: return		unicode_range(0x1F00, 0x1FFF);
		case unicode_block::General_Punctuation: return		unicode_range(0x2000, 0x206F);
		case unicode_block::Superscripts_and_Subscripts: return		unicode_range(0x2070, 0x209F);
		case unicode_block::Currency_Symbols: return		unicode_range(0x20A0, 0x20CF);
		case unicode_block::Combining_Diacritical_Marks_for_Symbols: return		unicode_range(0x20D0, 0x20FF);
		case unicode_block::Letterlike_Symbols: return		unicode_range(0x2100, 0x214F);
		case unicode_block::Number_Forms: return		unicode_range(0x2150, 0x218F);
		case unicode_block::Arrows: return		unicode_range(0x2190, 0x21FF);
		case unicode_block::Mathematical_Operators: return		unicode_range(0x2200, 0x22FF);
		case unicode_block::Miscellaneous_Technical: return		unicode_range(0x2300, 0x23FF);
		case unicode_block::Control_Pictures: return		unicode_range(0x2400, 0x243F);
		case unicode_block::Optical_Character_Recognition: return		unicode_range(0x2440, 0x245F);
		case unicode_block::Enclosed_Alphanumerics: return		unicode_range(0x2460, 0x24FF);
		case unicode_block::Box_Drawing: return		unicode_range(0x2500, 0x257F);
		case unicode_block::Block_Elements: return		unicode_range(0x2580, 0x259F);
		case unicode_block::Geometric_Shapes: return		unicode_range(0x25A0, 0x25FF);
		case unicode_block::Miscellaneous_Symbols: return		unicode_range(0x2600, 0x26FF);
		case unicode_block::Dingbats: return		unicode_range(0x2700, 0x27BF);
		case unicode_block::Miscellaneous_Mathematical_Symbols_A: return		unicode_range(0x27C0, 0x27EF);
		case unicode_block::Supplemental_Arrows_A: return		unicode_range(0x27F0, 0x27FF);
		case unicode_block::Braille_Patterns: return		unicode_range(0x2800, 0x28FF);
		case unicode_block::Supplemental_Arrows_B: return		unicode_range(0x2900, 0x297F);
		case unicode_block::Miscellaneous_Mathematical_Symbols_B: return		unicode_range(0x2980, 0x29FF);
		case unicode_block::Supplemental_Mathematical_Operators: return		unicode_range(0x2A00, 0x2AFF);
		case unicode_block::Miscellaneous_Symbols_and_Arrows: return		unicode_range(0x2B00, 0x2BFF);
		case unicode_block::Glagolitic: return		unicode_range(0x2C00, 0x2C5F);
		case unicode_block::Latin_Extended_C: return		unicode_range(0x2C60, 0x2C7F);
		case unicode_block::Coptic: return		unicode_range(0x2C80, 0x2CFF);
		case unicode_block::Georgian_Supplement: return		unicode_range(0x2D00, 0x2D2F);
		case unicode_block::Tifinagh: return		unicode_range(0x2D30, 0x2D7F);
		case unicode_block::Ethiopic_Extended: return		unicode_range(0x2D80, 0x2DDF);
		case unicode_block::Cyrillic_Extended_A: return		unicode_range(0x2DE0, 0x2DFF);
		case unicode_block::Supplemental_Punctuation: return		unicode_range(0x2E00, 0x2E7F);
		case unicode_block::CJK_Radicals_Supplement: return		unicode_range(0x2E80, 0x2EFF);
		case unicode_block::Kangxi_Radicals: return		unicode_range(0x2F00, 0x2FDF);
		case unicode_block::Ideographic_Description_Characters: return		unicode_range(0x2FF0, 0x2FFF);
		case unicode_block::CJK_Symbols_and_Punctuation: return		unicode_range(0x3000, 0x303F);
		case unicode_block::Hiragana: return		unicode_range(0x3040, 0x309F);
		case unicode_block::Katakana: return		unicode_range(0x30A0, 0x30FF);
		case unicode_block::Bopomofo: return		unicode_range(0x3100, 0x312F);
		case unicode_block::Hangul_Compatibility_Jamo: return		unicode_range(0x3130, 0x318F);
		case unicode_block::Kanbun: return		unicode_range(0x3190, 0x319F);
		case unicode_block::Bopomofo_Extended: return		unicode_range(0x31A0, 0x31BF);
		case unicode_block::CJK_Strokes: return		unicode_range(0x31C0, 0x31EF);
		case unicode_block::Katakana_Phonetic_Extensions: return		unicode_range(0x31F0, 0x31FF);
		case unicode_block::Enclosed_CJK_Letters_and_Months: return		unicode_range(0x3200, 0x32FF);
		case unicode_block::CJK_Compatibility: return		unicode_range(0x3300, 0x33FF);
		case unicode_block::CJK_Unified_Ideographs_Extension_A: return		unicode_range(0x3400, 0x4DBF);
		case unicode_block::Yijing_Hexagram_Symbols: return		unicode_range(0x4DC0, 0x4DFF);
		case unicode_block::CJK_Unified_Ideographs: return		unicode_range(0x4E00, 0x9FFF);
		case unicode_block::Yi_Syllables: return		unicode_range(0xA000, 0xA48F);
		case unicode_block::Yi_Radicals: return		unicode_range(0xA490, 0xA4CF);
		case unicode_block::Lisu: return		unicode_range(0xA4D0, 0xA4FF);
		case unicode_block::Vai: return		unicode_range(0xA500, 0xA63F);
		case unicode_block::Cyrillic_Extended_B: return		unicode_range(0xA640, 0xA69F);
		case unicode_block::Bamum: return		unicode_range(0xA6A0, 0xA6FF);
		case unicode_block::Modifier_Tone_Letters: return		unicode_range(0xA700, 0xA71F);
		case unicode_block::Latin_Extended_D: return		unicode_range(0xA720, 0xA7FF);
		case unicode_block::Syloti_Nagri: return		unicode_range(0xA800, 0xA82F);
		case unicode_block::Common_Indic_Number_Forms: return		unicode_range(0xA830, 0xA83F);
		case unicode_block::Phags_pa: return		unicode_range(0xA840, 0xA87F);
		case unicode_block::Saurashtra: return		unicode_range(0xA880, 0xA8DF);
		case unicode_block::Devanagari_Extended: return		unicode_range(0xA8E0, 0xA8FF);
		case unicode_block::Kayah_Li: return		unicode_range(0xA900, 0xA92F);
		case unicode_block::Rejang: return		unicode_range(0xA930, 0xA95F);
		case unicode_block::Hangul_Jamo_Extended_A: return		unicode_range(0xA960, 0xA97F);
		case unicode_block::Javanese: return		unicode_range(0xA980, 0xA9DF);
		case unicode_block::Cham: return		unicode_range(0xAA00, 0xAA5F);
		case unicode_block::Myanmar_Extended_A: return		unicode_range(0xAA60, 0xAA7F);
		case unicode_block::Tai_Viet: return		unicode_range(0xAA80, 0xAADF);
		case unicode_block::Meetei_Mayek_Extensions: return		unicode_range(0xAAE0, 0xAAFF);
		case unicode_block::Ethiopic_Extended_A: return		unicode_range(0xAB00, 0xAB2F);
		case unicode_block::Meetei_Mayek: return		unicode_range(0xABC0, 0xABFF);
		case unicode_block::Hangul_Syllables: return		unicode_range(0xAC00, 0xD7AF);
		case unicode_block::Hangul_Jamo_Extended_B: return		unicode_range(0xD7B0, 0xD7FF);
		case unicode_block::High_Surrogates: return		unicode_range(0xD800, 0xDB7F);
		case unicode_block::High_Private_Use_Surrogates: return		unicode_range(0xDB80, 0xDBFF);
		case unicode_block::Low_Surrogates: return		unicode_range(0xDC00, 0xDFFF);
		case unicode_block::Private_Use_Area: return		unicode_range(0xE000, 0xF8FF);
		case unicode_block::CJK_Compatibility_Ideographs: return		unicode_range(0xF900, 0xFAFF);
		case unicode_block::Alphabetic_Presentation_Forms: return		unicode_range(0xFB00, 0xFB4F);
		case unicode_block::Arabic_Presentation_Forms_A: return		unicode_range(0xFB50, 0xFDFF);
		case unicode_block::Variation_Selectors: return		unicode_range(0xFE00, 0xFE0F);
		case unicode_block::Vertical_Forms: return		unicode_range(0xFE10, 0xFE1F);
		case unicode_block::Combining_Half_Marks: return		unicode_range(0xFE20, 0xFE2F);
		case unicode_block::CJK_Compatibility_Forms: return		unicode_range(0xFE30, 0xFE4F);
		case unicode_block::Small_Form_Variants: return		unicode_range(0xFE50, 0xFE6F);
		case unicode_block::Arabic_Presentation_Forms_B: return		unicode_range(0xFE70, 0xFEFF);
		case unicode_block::Halfwidth_and_Fullwidth_Forms: return		unicode_range(0xFF00, 0xFFEF);
		case unicode_block::Specials: return		unicode_range(0xFFF0, 0xFFFF);
		case unicode_block::Linear_B_Syllabary: return		unicode_range(0x10000, 0x1007F);
		case unicode_block::Linear_B_Ideograms: return		unicode_range(0x10080, 0x100FF);
		case unicode_block::Aegean_Numbers: return		unicode_range(0x10100, 0x1013F);
		case unicode_block::Ancient_Greek_Numbers: return		unicode_range(0x10140, 0x1018F);
		case unicode_block::Ancient_Symbols: return		unicode_range(0x10190, 0x101CF);
		case unicode_block::Phaistos_Disc: return		unicode_range(0x101D0, 0x101FF);
		case unicode_block::Lycian: return		unicode_range(0x10280, 0x1029F);
		case unicode_block::Carian: return		unicode_range(0x102A0, 0x102DF);
		case unicode_block::Old_Italic: return		unicode_range(0x10300, 0x1032F);
		case unicode_block::Gothic: return		unicode_range(0x10330, 0x1034F);
		case unicode_block::Ugaritic: return		unicode_range(0x10380, 0x1039F);
		case unicode_block::Old_Persian: return		unicode_range(0x103A0, 0x103DF);
		case unicode_block::Deseret: return		unicode_range(0x10400, 0x1044F);
		case unicode_block::Shavian: return		unicode_range(0x10450, 0x1047F);
		case unicode_block::Osmanya: return		unicode_range(0x10480, 0x104AF);
		case unicode_block::Cypriot_Syllabary: return		unicode_range(0x10800, 0x1083F);
		case unicode_block::Imperial_Aramaic: return		unicode_range(0x10840, 0x1085F);
		case unicode_block::Phoenician: return		unicode_range(0x10900, 0x1091F);
		case unicode_block::Lydian: return		unicode_range(0x10920, 0x1093F);
		case unicode_block::Meroitic_Hieroglyphs: return		unicode_range(0x10980, 0x1099F);
		case unicode_block::Meroitic_Cursive: return		unicode_range(0x109A0, 0x109FF);
		case unicode_block::Kharoshthi: return		unicode_range(0x10A00, 0x10A5F);
		case unicode_block::Old_South_Arabian: return		unicode_range(0x10A60, 0x10A7F);
		case unicode_block::Avestan: return		unicode_range(0x10B00, 0x10B3F);
		case unicode_block::Inscriptional_Parthian: return		unicode_range(0x10B40, 0x10B5F);
		case unicode_block::Inscriptional_Pahlavi: return		unicode_range(0x10B60, 0x10B7F);
		case unicode_block::Old_Turkic: return		unicode_range(0x10C00, 0x10C4F);
		case unicode_block::Rumi_Numeral_Symbols: return		unicode_range(0x10E60, 0x10E7F);
		case unicode_block::Brahmi: return		unicode_range(0x11000, 0x1107F);
		case unicode_block::Kaithi: return		unicode_range(0x11080, 0x110CF);
		case unicode_block::Sora_Sompeng: return		unicode_range(0x110D0, 0x110FF);
		case unicode_block::Chakma: return		unicode_range(0x11100, 0x1114F);
		case unicode_block::Sharada: return		unicode_range(0x11180, 0x111DF);
		case unicode_block::Takri: return		unicode_range(0x11680, 0x116CF);
		case unicode_block::Cuneiform: return		unicode_range(0x12000, 0x123FF);
		case unicode_block::Cuneiform_Numbers_and_Punctuation: return		unicode_range(0x12400, 0x1247F);
		case unicode_block::Egyptian_Hieroglyphs: return		unicode_range(0x13000, 0x1342F);
		case unicode_block::Bamum_Supplement: return		unicode_range(0x16800, 0x16A3F);
		case unicode_block::Miao: return		unicode_range(0x16F00, 0x16F9F);
		case unicode_block::Kana_Supplement: return		unicode_range(0x1B000, 0x1B0FF);
		case unicode_block::Byzantine_Musical_Symbols: return		unicode_range(0x1D000, 0x1D0FF);
		case unicode_block::Musical_Symbols: return		unicode_range(0x1D100, 0x1D1FF);
		case unicode_block::Ancient_Greek_Musical_Notation: return		unicode_range(0x1D200, 0x1D24F);
		case unicode_block::Tai_Xuan_Jing_Symbols: return		unicode_range(0x1D300, 0x1D35F);
		case unicode_block::Counting_Rod_Numerals: return		unicode_range(0x1D360, 0x1D37F);
		case unicode_block::Mathematical_Alphanumeric_Symbols: return		unicode_range(0x1D400, 0x1D7FF);
		case unicode_block::Arabic_Mathematical_Alphabetic_Symbols: return		unicode_range(0x1EE00, 0x1EEFF);
		case unicode_block::Mahjong_Tiles: return		unicode_range(0x1F000, 0x1F02F);
		case unicode_block::Domino_Tiles: return		unicode_range(0x1F030, 0x1F09F);
		case unicode_block::Playing_Cards: return		unicode_range(0x1F0A0, 0x1F0FF);
		case unicode_block::Enclosed_Alphanumeric_Supplement: return		unicode_range(0x1F100, 0x1F1FF);
		case unicode_block::Enclosed_Ideographic_Supplement: return		unicode_range(0x1F200, 0x1F2FF);
		case unicode_block::Miscellaneous_Symbols_And_Pictographs: return		unicode_range(0x1F300, 0x1F5FF);
		case unicode_block::Emoticons: return		unicode_range(0x1F600, 0x1F64F);
		case unicode_block::Transport_And_Map_Symbols: return		unicode_range(0x1F680, 0x1F6FF);
		case unicode_block::Alchemical_Symbols: return		unicode_range(0x1F700, 0x1F77F);
		case unicode_block::CJK_Unified_Ideographs_Extension_B: return		unicode_range(0x20000, 0x2A6DF);
		case unicode_block::CJK_Unified_Ideographs_Extension_C: return		unicode_range(0x2A700, 0x2B73F);
		case unicode_block::CJK_Unified_Ideographs_Extension_D: return		unicode_range(0x2B740, 0x2B81F);
		case unicode_block::CJK_Compatibility_Ideographs_Supplement: return		unicode_range(0x2F800, 0x2FA1F);
		case unicode_block::Tags: return		unicode_range(0xE0000, 0xE007F);
		case unicode_block::Variation_Selectors_Supplement: return		unicode_range(0xE0100, 0xE01EF);
		case unicode_block::Supplementary_Private_Use_Area_A: return		unicode_range(0xF0000, 0xFFFFF);
		case unicode_block::Supplementary_Private_Use_Area_B: return		unicode_range(0x100000, 0x10FFFF);
	}
	return unicode_range(0,0);
}

}}}