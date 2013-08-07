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

	FT_Library		lib_;
	FT_Face			face_;
	texture_atlas	atlas_;
	float			size_;
	bool			normalize_;
	std::map<int, glyph_info> glyphs_;

public:
	impl::impl(texture_atlas& atlas, const std::wstring& filename, float size, bool normalize_coordinates) : lib_(nullptr), face_(nullptr), atlas_(atlas), size_(size), normalize_(normalize_coordinates)
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
		int maxHeight = 0;

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

				int bearingY = face_->glyph->metrics.horiBearingY >> 6;
				if(bearingY > maxBearingY)
					maxBearingY = bearingY;

				if(coords.height > maxHeight)
					maxHeight = coords.height;

				pos_x += face_->glyph->advance.x / 64.0f;
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

texture_font::texture_font(texture_atlas& atlas, const std::wstring& filename, float size, bool normalize_coordinates) : impl_(new impl(atlas, filename, size, normalize_coordinates)) {}
void texture_font::load_glyphs(unicode_block range, const color<float>& col) { impl_->load_glyphs(range, col); }
std::vector<float> texture_font::create_vertex_stream(const std::wstring& str, int x, int y, int parent_width, int parent_height, string_metrics* metrics) { return impl_->create_vertex_stream(str, x, y, parent_width, parent_height, metrics); }
string_metrics texture_font::measure_string(const std::wstring& str) { return impl_->measure_string(str); }

unicode_range get_range(unicode_block block)
{
	switch(block)
	{
		case Basic_Latin: return unicode_range(0x0000, 0x007F); 
		case Latin_1_Supplement: return unicode_range(0x0080, 0x00FF); 
		case Latin_Extended_A: return		unicode_range(0x0100, 0x017F); 
		case Latin_Extended_B: return		unicode_range(0x0180, 0x024F); 
		case IPA_Extensions: return		unicode_range(0x0250, 0x02AF); 
		case Spacing_Modifier_Letters: return		unicode_range(0x02B0, 0x02FF); 
		case Combining_Diacritical_Marks: return		unicode_range(0x0300, 0x036F); 
		case Greek_and_Coptic: return		unicode_range(0x0370, 0x03FF); 
		case Cyrillic: return		unicode_range(0x0400, 0x04FF); 
		case Cyrillic_Supplement: return		unicode_range(0x0500, 0x052F); 
		case Armenian: return		unicode_range(0x0530, 0x058F); 
		case Hebrew: return		unicode_range(0x0590, 0x05FF); 
		case Arabic: return		unicode_range(0x0600, 0x06FF); 
		case Syriac: return		unicode_range(0x0700, 0x074F); 
		case Arabic_Supplement: return		unicode_range(0x0750, 0x077F); 
		case Thaana: return		unicode_range(0x0780, 0x07BF); 
		case NKo: return		unicode_range(0x07C0, 0x07FF); 
		case Samaritan: return		unicode_range(0x0800, 0x083F); 
		case Mandaic: return		unicode_range(0x0840, 0x085F); 
		case Arabic_Extended_A: return		unicode_range(0x08A0, 0x08FF); 
		case Devanagari: return		unicode_range(0x0900, 0x097F); 
		case Bengali: return		unicode_range(0x0980, 0x09FF); 
		case Gurmukhi: return		unicode_range(0x0A00, 0x0A7F); 
		case Gujarati: return		unicode_range(0x0A80, 0x0AFF); 
		case Oriya: return		unicode_range(0x0B00, 0x0B7F); 
		case Tamil: return		unicode_range(0x0B80, 0x0BFF); 
		case Telugu: return		unicode_range(0x0C00, 0x0C7F); 
		case Kannada: return		unicode_range(0x0C80, 0x0CFF); 
		case Malayalam: return		unicode_range(0x0D00, 0x0D7F); 
		case Sinhala: return		unicode_range(0x0D80, 0x0DFF); 
		case Thai: return		unicode_range(0x0E00, 0x0E7F); 
		case Lao: return		unicode_range(0x0E80, 0x0EFF); 
		case Tibetan: return		unicode_range(0x0F00, 0x0FFF); 
		case Myanmar: return		unicode_range(0x1000, 0x109F); 
		case Georgian: return		unicode_range(0x10A0, 0x10FF); 
		case Hangul_Jamo: return		unicode_range(0x1100, 0x11FF); 
		case Ethiopic: return		unicode_range(0x1200, 0x137F); 
		case Ethiopic_Supplement: return		unicode_range(0x1380, 0x139F); 
		case Cherokee: return		unicode_range(0x13A0, 0x13FF); 
		case Unified_Canadian_Aboriginal_Syllabics: return		unicode_range(0x1400, 0x167F); 
		case Ogham: return		unicode_range(0x1680, 0x169F); 
		case Runic: return		unicode_range(0x16A0, 0x16FF); 
		case Tagalog: return		unicode_range(0x1700, 0x171F); 
		case Hanunoo: return		unicode_range(0x1720, 0x173F); 
		case Buhid: return		unicode_range(0x1740, 0x175F); 
		case Tagbanwa: return		unicode_range(0x1760, 0x177F); 
		case Khmer: return		unicode_range(0x1780, 0x17FF); 
		case Mongolian: return		unicode_range(0x1800, 0x18AF); 
		case Unified_Canadian_Aboriginal_Syllabics_Extended: return		unicode_range(0x18B0, 0x18FF); 
		case Limbu: return		unicode_range(0x1900, 0x194F); 
		case Tai_Le: return		unicode_range(0x1950, 0x197F); 
		case New_Tai_Lue: return		unicode_range(0x1980, 0x19DF); 
		case Khmer_Symbols: return		unicode_range(0x19E0, 0x19FF); 
		case Buginese: return		unicode_range(0x1A00, 0x1A1F); 
		case Tai_Tham: return		unicode_range(0x1A20, 0x1AAF); 
		case Balinese: return		unicode_range(0x1B00, 0x1B7F); 
		case Sundanese: return		unicode_range(0x1B80, 0x1BBF); 
		case Batak: return		unicode_range(0x1BC0, 0x1BFF); 
		case Lepcha: return		unicode_range(0x1C00, 0x1C4F); 
		case Ol_Chiki: return		unicode_range(0x1C50, 0x1C7F); 
		case Sundanese_Supplement: return		unicode_range(0x1CC0, 0x1CCF); 
		case Vedic_Extensions: return		unicode_range(0x1CD0, 0x1CFF); 
		case Phonetic_Extensions: return		unicode_range(0x1D00, 0x1D7F); 
		case Phonetic_Extensions_Supplement: return		unicode_range(0x1D80, 0x1DBF); 
		case Combining_Diacritical_Marks_Supplement: return		unicode_range(0x1DC0, 0x1DFF); 
		case Latin_Extended_Additional: return		unicode_range(0x1E00, 0x1EFF); 
		case Greek_Extended: return		unicode_range(0x1F00, 0x1FFF); 
		case General_Punctuation: return		unicode_range(0x2000, 0x206F); 
		case Superscripts_and_Subscripts: return		unicode_range(0x2070, 0x209F); 
		case Currency_Symbols: return		unicode_range(0x20A0, 0x20CF); 
		case Combining_Diacritical_Marks_for_Symbols: return		unicode_range(0x20D0, 0x20FF); 
		case Letterlike_Symbols: return		unicode_range(0x2100, 0x214F); 
		case Number_Forms: return		unicode_range(0x2150, 0x218F); 
		case Arrows: return		unicode_range(0x2190, 0x21FF); 
		case Mathematical_Operators: return		unicode_range(0x2200, 0x22FF); 
		case Miscellaneous_Technical: return		unicode_range(0x2300, 0x23FF); 
		case Control_Pictures: return		unicode_range(0x2400, 0x243F); 
		case Optical_Character_Recognition: return		unicode_range(0x2440, 0x245F); 
		case Enclosed_Alphanumerics: return		unicode_range(0x2460, 0x24FF); 
		case Box_Drawing: return		unicode_range(0x2500, 0x257F); 
		case Block_Elements: return		unicode_range(0x2580, 0x259F); 
		case Geometric_Shapes: return		unicode_range(0x25A0, 0x25FF); 
		case Miscellaneous_Symbols: return		unicode_range(0x2600, 0x26FF); 
		case Dingbats: return		unicode_range(0x2700, 0x27BF); 
		case Miscellaneous_Mathematical_Symbols_A: return		unicode_range(0x27C0, 0x27EF); 
		case Supplemental_Arrows_A: return		unicode_range(0x27F0, 0x27FF); 
		case Braille_Patterns: return		unicode_range(0x2800, 0x28FF); 
		case Supplemental_Arrows_B: return		unicode_range(0x2900, 0x297F); 
		case Miscellaneous_Mathematical_Symbols_B: return		unicode_range(0x2980, 0x29FF); 
		case Supplemental_Mathematical_Operators: return		unicode_range(0x2A00, 0x2AFF); 
		case Miscellaneous_Symbols_and_Arrows: return		unicode_range(0x2B00, 0x2BFF); 
		case Glagolitic: return		unicode_range(0x2C00, 0x2C5F); 
		case Latin_Extended_C: return		unicode_range(0x2C60, 0x2C7F); 
		case Coptic: return		unicode_range(0x2C80, 0x2CFF); 
		case Georgian_Supplement: return		unicode_range(0x2D00, 0x2D2F); 
		case Tifinagh: return		unicode_range(0x2D30, 0x2D7F); 
		case Ethiopic_Extended: return		unicode_range(0x2D80, 0x2DDF); 
		case Cyrillic_Extended_A: return		unicode_range(0x2DE0, 0x2DFF); 
		case Supplemental_Punctuation: return		unicode_range(0x2E00, 0x2E7F); 
		case CJK_Radicals_Supplement: return		unicode_range(0x2E80, 0x2EFF); 
		case Kangxi_Radicals: return		unicode_range(0x2F00, 0x2FDF); 
		case Ideographic_Description_Characters: return		unicode_range(0x2FF0, 0x2FFF); 
		case CJK_Symbols_and_Punctuation: return		unicode_range(0x3000, 0x303F); 
		case Hiragana: return		unicode_range(0x3040, 0x309F); 
		case Katakana: return		unicode_range(0x30A0, 0x30FF); 
		case Bopomofo: return		unicode_range(0x3100, 0x312F); 
		case Hangul_Compatibility_Jamo: return		unicode_range(0x3130, 0x318F); 
		case Kanbun: return		unicode_range(0x3190, 0x319F); 
		case Bopomofo_Extended: return		unicode_range(0x31A0, 0x31BF); 
		case CJK_Strokes: return		unicode_range(0x31C0, 0x31EF); 
		case Katakana_Phonetic_Extensions: return		unicode_range(0x31F0, 0x31FF); 
		case Enclosed_CJK_Letters_and_Months: return		unicode_range(0x3200, 0x32FF); 
		case CJK_Compatibility: return		unicode_range(0x3300, 0x33FF); 
		case CJK_Unified_Ideographs_Extension_A: return		unicode_range(0x3400, 0x4DBF); 
		case Yijing_Hexagram_Symbols: return		unicode_range(0x4DC0, 0x4DFF); 
		case CJK_Unified_Ideographs: return		unicode_range(0x4E00, 0x9FFF); 
		case Yi_Syllables: return		unicode_range(0xA000, 0xA48F); 
		case Yi_Radicals: return		unicode_range(0xA490, 0xA4CF); 
		case Lisu: return		unicode_range(0xA4D0, 0xA4FF); 
		case Vai: return		unicode_range(0xA500, 0xA63F); 
		case Cyrillic_Extended_B: return		unicode_range(0xA640, 0xA69F); 
		case Bamum: return		unicode_range(0xA6A0, 0xA6FF); 
		case Modifier_Tone_Letters: return		unicode_range(0xA700, 0xA71F); 
		case Latin_Extended_D: return		unicode_range(0xA720, 0xA7FF); 
		case Syloti_Nagri: return		unicode_range(0xA800, 0xA82F); 
		case Common_Indic_Number_Forms: return		unicode_range(0xA830, 0xA83F); 
		case Phags_pa: return		unicode_range(0xA840, 0xA87F); 
		case Saurashtra: return		unicode_range(0xA880, 0xA8DF); 
		case Devanagari_Extended: return		unicode_range(0xA8E0, 0xA8FF); 
		case Kayah_Li: return		unicode_range(0xA900, 0xA92F); 
		case Rejang: return		unicode_range(0xA930, 0xA95F); 
		case Hangul_Jamo_Extended_A: return		unicode_range(0xA960, 0xA97F); 
		case Javanese: return		unicode_range(0xA980, 0xA9DF); 
		case Cham: return		unicode_range(0xAA00, 0xAA5F); 
		case Myanmar_Extended_A: return		unicode_range(0xAA60, 0xAA7F); 
		case Tai_Viet: return		unicode_range(0xAA80, 0xAADF); 
		case Meetei_Mayek_Extensions: return		unicode_range(0xAAE0, 0xAAFF); 
		case Ethiopic_Extended_A: return		unicode_range(0xAB00, 0xAB2F); 
		case Meetei_Mayek: return		unicode_range(0xABC0, 0xABFF); 
		case Hangul_Syllables: return		unicode_range(0xAC00, 0xD7AF); 
		case Hangul_Jamo_Extended_B: return		unicode_range(0xD7B0, 0xD7FF); 
		case High_Surrogates: return		unicode_range(0xD800, 0xDB7F); 
		case High_Private_Use_Surrogates: return		unicode_range(0xDB80, 0xDBFF); 
		case Low_Surrogates: return		unicode_range(0xDC00, 0xDFFF); 
		case Private_Use_Area: return		unicode_range(0xE000, 0xF8FF); 
		case CJK_Compatibility_Ideographs: return		unicode_range(0xF900, 0xFAFF); 
		case Alphabetic_Presentation_Forms: return		unicode_range(0xFB00, 0xFB4F); 
		case Arabic_Presentation_Forms_A: return		unicode_range(0xFB50, 0xFDFF); 
		case Variation_Selectors: return		unicode_range(0xFE00, 0xFE0F); 
		case Vertical_Forms: return		unicode_range(0xFE10, 0xFE1F); 
		case Combining_Half_Marks: return		unicode_range(0xFE20, 0xFE2F); 
		case CJK_Compatibility_Forms: return		unicode_range(0xFE30, 0xFE4F); 
		case Small_Form_Variants: return		unicode_range(0xFE50, 0xFE6F); 
		case Arabic_Presentation_Forms_B: return		unicode_range(0xFE70, 0xFEFF); 
		case Halfwidth_and_Fullwidth_Forms: return		unicode_range(0xFF00, 0xFFEF); 
		case Specials: return		unicode_range(0xFFF0, 0xFFFF); 
		case Linear_B_Syllabary: return		unicode_range(0x10000, 0x1007F); 
		case Linear_B_Ideograms: return		unicode_range(0x10080, 0x100FF); 
		case Aegean_Numbers: return		unicode_range(0x10100, 0x1013F); 
		case Ancient_Greek_Numbers: return		unicode_range(0x10140, 0x1018F); 
		case Ancient_Symbols: return		unicode_range(0x10190, 0x101CF); 
		case Phaistos_Disc: return		unicode_range(0x101D0, 0x101FF); 
		case Lycian: return		unicode_range(0x10280, 0x1029F); 
		case Carian: return		unicode_range(0x102A0, 0x102DF); 
		case Old_Italic: return		unicode_range(0x10300, 0x1032F); 
		case Gothic: return		unicode_range(0x10330, 0x1034F); 
		case Ugaritic: return		unicode_range(0x10380, 0x1039F); 
		case Old_Persian: return		unicode_range(0x103A0, 0x103DF); 
		case Deseret: return		unicode_range(0x10400, 0x1044F); 
		case Shavian: return		unicode_range(0x10450, 0x1047F); 
		case Osmanya: return		unicode_range(0x10480, 0x104AF); 
		case Cypriot_Syllabary: return		unicode_range(0x10800, 0x1083F); 
		case Imperial_Aramaic: return		unicode_range(0x10840, 0x1085F); 
		case Phoenician: return		unicode_range(0x10900, 0x1091F); 
		case Lydian: return		unicode_range(0x10920, 0x1093F); 
		case Meroitic_Hieroglyphs: return		unicode_range(0x10980, 0x1099F); 
		case Meroitic_Cursive: return		unicode_range(0x109A0, 0x109FF); 
		case Kharoshthi: return		unicode_range(0x10A00, 0x10A5F); 
		case Old_South_Arabian: return		unicode_range(0x10A60, 0x10A7F); 
		case Avestan: return		unicode_range(0x10B00, 0x10B3F); 
		case Inscriptional_Parthian: return		unicode_range(0x10B40, 0x10B5F); 
		case Inscriptional_Pahlavi: return		unicode_range(0x10B60, 0x10B7F); 
		case Old_Turkic: return		unicode_range(0x10C00, 0x10C4F); 
		case Rumi_Numeral_Symbols: return		unicode_range(0x10E60, 0x10E7F); 
		case Brahmi: return		unicode_range(0x11000, 0x1107F); 
		case Kaithi: return		unicode_range(0x11080, 0x110CF); 
		case Sora_Sompeng: return		unicode_range(0x110D0, 0x110FF); 
		case Chakma: return		unicode_range(0x11100, 0x1114F); 
		case Sharada: return		unicode_range(0x11180, 0x111DF); 
		case Takri: return		unicode_range(0x11680, 0x116CF); 
		case Cuneiform: return		unicode_range(0x12000, 0x123FF); 
		case Cuneiform_Numbers_and_Punctuation: return		unicode_range(0x12400, 0x1247F); 
		case Egyptian_Hieroglyphs: return		unicode_range(0x13000, 0x1342F); 
		case Bamum_Supplement: return		unicode_range(0x16800, 0x16A3F); 
		case Miao: return		unicode_range(0x16F00, 0x16F9F); 
		case Kana_Supplement: return		unicode_range(0x1B000, 0x1B0FF); 
		case Byzantine_Musical_Symbols: return		unicode_range(0x1D000, 0x1D0FF); 
		case Musical_Symbols: return		unicode_range(0x1D100, 0x1D1FF); 
		case Ancient_Greek_Musical_Notation: return		unicode_range(0x1D200, 0x1D24F); 
		case Tai_Xuan_Jing_Symbols: return		unicode_range(0x1D300, 0x1D35F); 
		case Counting_Rod_Numerals: return		unicode_range(0x1D360, 0x1D37F); 
		case Mathematical_Alphanumeric_Symbols: return		unicode_range(0x1D400, 0x1D7FF); 
		case Arabic_Mathematical_Alphabetic_Symbols: return		unicode_range(0x1EE00, 0x1EEFF); 
		case Mahjong_Tiles: return		unicode_range(0x1F000, 0x1F02F); 
		case Domino_Tiles: return		unicode_range(0x1F030, 0x1F09F); 
		case Playing_Cards: return		unicode_range(0x1F0A0, 0x1F0FF); 
		case Enclosed_Alphanumeric_Supplement: return		unicode_range(0x1F100, 0x1F1FF); 
		case Enclosed_Ideographic_Supplement: return		unicode_range(0x1F200, 0x1F2FF); 
		case Miscellaneous_Symbols_And_Pictographs: return		unicode_range(0x1F300, 0x1F5FF); 
		case Emoticons: return		unicode_range(0x1F600, 0x1F64F); 
		case Transport_And_Map_Symbols: return		unicode_range(0x1F680, 0x1F6FF); 
		case Alchemical_Symbols: return		unicode_range(0x1F700, 0x1F77F); 
		case CJK_Unified_Ideographs_Extension_B: return		unicode_range(0x20000, 0x2A6DF); 
		case CJK_Unified_Ideographs_Extension_C: return		unicode_range(0x2A700, 0x2B73F); 
		case CJK_Unified_Ideographs_Extension_D: return		unicode_range(0x2B740, 0x2B81F); 
		case CJK_Compatibility_Ideographs_Supplement: return		unicode_range(0x2F800, 0x2FA1F); 
		case Tags: return		unicode_range(0xE0000, 0xE007F); 
		case Variation_Selectors_Supplement: return		unicode_range(0xE0100, 0xE01EF); 
		case Supplementary_Private_Use_Area_A: return		unicode_range(0xF0000, 0xFFFFF); 
		case Supplementary_Private_Use_Area_B: return		unicode_range(0x100000, 0x10FFFF);
	}
	return unicode_range(0,0);
}

}}}