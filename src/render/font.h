#ifndef RENDER_FONT_H
#define RENDER_FONT_H

#include "core/types.h"
#include "core/colour.h"
#include "render/canvas.h"

#include "container/hash_map.h"
#include <memory>
#include "container/string.h"
#include "container/vector.h"

namespace rnd {

class Image;

enum TextAlign {
	TextAlign_Left,
	TextAlign_Centre,
	TextAlign_Right
};

struct FontGlyph {
	int x = 0; // in the page
	int y = 0;
	int width = 0;
	int height = 0;
	int offset_x = 0;
	int offset_y = 0; // from the top of the line downwards
	int advance = 0;
	int page = 0;
};

class Font {
public:
	~Font();

	Font(const Font &) = delete;
	Font &operator=(const Font &) = delete;

	static Unique<Font> LoadBMFont(const String &path);
	static Unique<Font> CreateDefault();

	void SetColour(const Colour &colour);
	void SetColour(float r, float g, float b, float a);
	const Colour &GetColour() const;

	void SetScale(float scale);
	float GetScale() const;

	void SetUseIntegerPositions(bool enabled);

	float GetLineHeight() const;
	float GetCapHeight() const;

	const FontGlyph *GetGlyph(unsigned codepoint) const;
	float GetKerning(unsigned first, unsigned second) const;

	float GetAdvanceAt(const String &text, u64 index) const;

	float MeasureWidth(const String &text) const;

	void Draw(Canvas *canvas, const String &text, float x, float y) const;
	void Draw(Canvas *canvas, const String &text, float x, float y, float target_width, TextAlign align) const;
	void DrawRange(Canvas *canvas, const String &text, u64 start, u64 end, float x, float y) const;

	int DrawWrapped(Canvas *canvas, const String &text, float x, float y, float target_width, TextAlign align) const;

private:
	Font();

	void ComputeCapTop();

	Vector<Unique<Image>> pages;
	HashMap<unsigned, FontGlyph> glyphs;
	HashMap<u64, int> kernings;

	float line_height;
	float cap_top; // top of the capitals from the top of the line
	float cap_height;

	Colour colour;
	float scale;
	bool integer_positions;
};

}

#endif // RENDER_FONT_H
