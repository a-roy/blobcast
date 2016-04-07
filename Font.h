#pragma once

#include <freetype-gl/freetype-gl.h>
#include <vector>
#include <string>

class Font
{
	public:
		texture_atlas_t *TextureAtlas;
		texture_font_t *TextureFont;

		Font(std::string path, float size);
		~Font();
		texture_glyph_t *GetGlyph(const char *c) const;
		float GetKerning(texture_glyph_t *glyph, const char *c) const;
		void UploadTextureAtlas(int texture_unit);
		void BindTexture(GLuint location);
};
