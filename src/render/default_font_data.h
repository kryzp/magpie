#ifndef RENDER_DEFAULT_FONT_DATA_H
#define RENDER_DEFAULT_FONT_DATA_H

#include "core/types.h"

namespace rnd {

struct DefaultFontGlyph {
	int codepoint;
	int x;
	int y;
	int width;
	int height;
	int offset_x;
	int offset_y;
	int advance;
};

namespace default_font {

extern const DefaultFontGlyph GLYPHS[];
extern const unsigned GLYPH_COUNT;
extern const unsigned ATLAS_WIDTH;
extern const unsigned ATLAS_HEIGHT;
extern const unsigned LINE_HEIGHT;
extern const unsigned ASCENT;

extern const u8 ATLAS_BITS[];

}

}

#endif // RENDER_DEFAULT_FONT_DATA_H
