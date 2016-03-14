#include "Font.h"

Font::Font(std::string path, float size)
{
	TextureAtlas = texture_atlas_new(512, 512, 1);
	TextureFont = texture_font_new_from_file(TextureAtlas, size, path.c_str());
	glGenTextures(1, &TextureAtlas->id);
}

Font::~Font()
{
	texture_font_delete(TextureFont);
	texture_atlas_delete(TextureAtlas);
}

texture_glyph_t *Font::GetGlyph(const char *c) const
{
	return texture_font_get_glyph(TextureFont, c);
}

float Font::GetKerning(texture_glyph_t *glyph, const char *c) const
{
	return texture_glyph_get_kerning(glyph, c);
}

void Font::UploadTextureAtlas()
{
	glActiveTexture(GL_TEXTURE0 + TextureAtlas->id - 1);
	texture_atlas_upload(TextureAtlas);
}

void Font::BindTexture(GLuint location)
{
	glUniform1i(location, TextureAtlas->id - 1);
}
