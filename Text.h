#pragma once

#include <GL/glew.h>
#include "VertexArray.h"
#include "Font.h"
#include "Buffer.h"
#include <vector>
#include <string>

class Text
{
	public:
		Font *FontStyle;
		VertexArray *VAO;
		FloatBuffer *Vertices;
		FloatBuffer *TexCoords;
		int NumVerts = 0;
		float XPosition = 0.f;
		float YPosition = 0.f;

		Text(Font *font);
		~Text();
		void Draw() const;
		void SetText(std::string text);
};
