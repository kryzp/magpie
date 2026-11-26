#include "render/font.h"
#include "render/image.h"
#include "render/default_font_data.h"

#include <stb/stb_image.h>

#include <cmath>
#include <fstream>
#include <sstream>

static u64 KerningKey(unsigned first, unsigned second)
{
	return
		(static_cast<u64>(first) << 32) |
		(static_cast<u64>(second) << 0);
}

// "key=value key2="quoted value"" -> map
static HashMap<String, String> ParseBMFontLine(const String &line, String *tag)
{
	HashMap<String, String> out;

	u64 i = 0;

	while (i < line.size() && line[i] != ' ') {
		i++;
	}

	*tag = line.substr(0, i);

	while (i < line.size()) {
		while (i < line.size() && line[i] == ' ')
			i++;

		u64 key_start = i;

		while (i < line.size() && line[i] != '=' && line[i] != ' ')
			i++;

		String key = line.substr(key_start, i - key_start);

		if (i >= line.size() || line[i] != '=')
			continue;

		i++;

		String value;

		if (i < line.size() && line[i] == '"') {
			i++;
			u64 value_start = i;

			while (i < line.size() && line[i] != '"')
				i++;

			value = line.substr(value_start, i - value_start);
			i++;
		} else {
			u64 value_start = i;

			while (i < line.size() && line[i] != ' ')
				i++;

			value = line.substr(value_start, i - value_start);
		}

		out[key] = value;
	}

	return out;
}

static int IntOf(const HashMap<String, String> &values, const char *key)
{
	auto it = values.find(key);

	if (it == values.end())
		return 0;

	return atoi(it->second.c_str());
}

rnd::Font::Font()
	: line_height(0.f)
	, cap_top(0.f)
	, cap_height(0.f)
	, colour(Colour::White())
	, scale(1.f)
	, integer_positions(true)
{
}

rnd::Font::~Font()
{
}

Unique<rnd::Font> rnd::Font::LoadBMFont(const String &path)
{
	std::ifstream file(path);

	if (!file)
		return nullptr;

	String directory;
	u64 slash = path.find_last_of("/\\");

	if (slash != String::npos)
		directory = path.substr(0, slash + 1);

	Unique<Font> font(new Font());

	String line;

	while (std::getline(file, line)) {
		if (!line.empty() && line.back() == '\r')
			line.pop_back();

		String tag;
		HashMap<String, String> values = ParseBMFontLine(line, &tag);

		if (tag == "common") {
			font->line_height = static_cast<float>(IntOf(values, "lineHeight"));
		} else if (tag == "page") {
			String file_name = values["file"];
			String page_path = directory + file_name;

			int w = 0;
			int h = 0;
			int n = 0;
			u8 *pixels = stbi_load(page_path.c_str(), &w, &h, &n, 4);

			if (!pixels)
				return nullptr;

			unsigned id = static_cast<unsigned>(IntOf(values, "id"));

			if (font->pages.size() <= id)
				font->pages.resize(id + 1);

			font->pages[id] = Image::Create(w, h, pixels);
			stbi_image_free(pixels);
		} else if (tag == "char") {
			FontGlyph g;
			g.x = IntOf(values, "x");
			g.y = IntOf(values, "y");
			g.width = IntOf(values, "width");
			g.height = IntOf(values, "height");
			g.offset_x = IntOf(values, "xoffset");
			g.offset_y = IntOf(values, "yoffset");
			g.advance = IntOf(values, "xadvance");
			g.page = IntOf(values, "page");

			font->glyphs[static_cast<unsigned>(IntOf(values, "id"))] = g;
		} else if (tag == "kerning") {
			unsigned first = static_cast<unsigned>(IntOf(values, "first"));
			unsigned second = static_cast<unsigned>(IntOf(values, "second"));

			font->kernings[KerningKey(first, second)] = IntOf(values, "amount");
		}
	}

	if (font->pages.empty() || font->glyphs.empty())
		return nullptr;

	font->ComputeCapTop();

	return font;
}

Unique<rnd::Font> rnd::Font::CreateDefault()
{
	unsigned w = default_font::ATLAS_WIDTH;
	unsigned h = default_font::ATLAS_HEIGHT;

	Vector<u8> pixels(static_cast<u64>(w) * h * 4, 0);

	for (unsigned i = 0; i < w * h; i++) {
		bool set = (default_font::ATLAS_BITS[i / 8] >> (i % 8)) & 1;

		pixels[i * 4 + 0] = 255;
		pixels[i * 4 + 1] = 255;
		pixels[i * 4 + 2] = 255;

		if (set)
			pixels[i * 4 + 3] = 255;
	}

	Unique<Font> font(new Font());

	font->pages.push_back(Image::Create(w, h, pixels.data()));
	font->line_height = static_cast<float>(default_font::LINE_HEIGHT);

	for (unsigned i = 0; i < default_font::GLYPH_COUNT; i++) {
		const DefaultFontGlyph &src = default_font::GLYPHS[i];

		FontGlyph g;
		g.x = src.x;
		g.y = src.y;
		g.width = src.width;
		g.height = src.height;
		g.offset_x = src.offset_x;
		g.offset_y = src.offset_y;
		g.advance = src.advance;
		g.page = 0;

		font->glyphs[src.codepoint] = g;
	}

	font->ComputeCapTop();

	return font;
}

void rnd::Font::ComputeCapTop()
{
	const char *capitals = "XEMHIKLNTUVWZ";

	for (const char *c = capitals; *c; c++) {
		const FontGlyph *g = GetGlyph(static_cast<unsigned>(*c));

		if (g && g->height > 0) {
			cap_top = static_cast<float>(g->offset_y);
			cap_height = static_cast<float>(g->height);
			return;
		}
	}

	cap_top = 0.f;
	cap_height = line_height;
}

void rnd::Font::SetColour(const Colour &colour_)
{
	colour = colour_;
}

void rnd::Font::SetColour(float r, float g, float b, float a)
{
	colour = Colour(r, g, b, a);
}

const Colour &rnd::Font::GetColour() const
{
	return colour;
}

void rnd::Font::SetScale(float scale_)
{
	scale = scale_;
}

float rnd::Font::GetScale() const
{
	return scale;
}

void rnd::Font::SetUseIntegerPositions(bool enabled)
{
	integer_positions = enabled;
}

float rnd::Font::GetLineHeight() const
{
	return line_height * scale;
}

float rnd::Font::GetCapHeight() const
{
	return cap_height * scale;
}

const rnd::FontGlyph *rnd::Font::GetGlyph(unsigned codepoint) const
{
	auto it = glyphs.find(codepoint);

	if (it == glyphs.end())
		return nullptr;

	return &it->second;
}

float rnd::Font::GetKerning(unsigned first, unsigned second) const
{
	auto it = kernings.find(KerningKey(first, second));

	if (it == kernings.end())
		return 0.f;

	return it->second;
}

float rnd::Font::GetAdvanceAt(const String &text, u64 index) const
{
	unsigned c = static_cast<u8>(text[index]);

	const FontGlyph *g = GetGlyph(c);

	if (!g)
		return 0.f;

	float a = g->advance;

	if (index + 1 < text.size())
		a += GetKerning(c, text[index + 1]);

	return a * scale;
}

float rnd::Font::MeasureWidth(const String &text) const
{
	float width = 0.f;

	for (u64 i = 0; i < text.size(); i++)
		width += GetAdvanceAt(text, i);

	return width;
}

void rnd::Font::Draw(Canvas *canvas, const String &text, float x, float y) const
{
	DrawRange(canvas, text, 0, text.size(), x, y);
}

void rnd::Font::Draw(Canvas *canvas, const String &text, float x, float y, float target_width, TextAlign align) const
{
	float width = MeasureWidth(text);
	float start_x = x;

	if (align == TextAlign_Centre)
		start_x = x + (target_width - width) * 0.5f;
	else if (align == TextAlign_Right)
		start_x = x + target_width - width;

	DrawRange(canvas, text, 0, text.size(), start_x, y);
}

int rnd::Font::DrawWrapped(Canvas *canvas, const String &text, float x, float y, float target_width, TextAlign align) const
{
	int lines = 0;
	u64 line_start = 0;

	while (line_start < text.size()) {
		u64 line_end = line_start;
		u64 last_break = String::npos;
		float width = 0.f;

		while (line_end < text.size()) {
			if (text[line_end] == '\n')
				break;

			float advance = GetAdvanceAt(text, line_end);

			if (width + advance > target_width && line_end > line_start)
				break;

			if (text[line_end] == ' ')
				last_break = line_end;

			width += advance;
			line_end++;
		}

		u64 next_start = line_end;

		if (line_end < text.size() && text[line_end] != '\n' && last_break != String::npos) {
			line_end = last_break;
			next_start = last_break + 1;
		} else if (line_end < text.size() && text[line_end] == '\n') {
			next_start = line_end + 1;
		}

		String line = text.substr(line_start, line_end - line_start);
		Draw(canvas, line, x, y - static_cast<float>(lines) * line_height * scale, target_width, align);

		lines++;
		line_start = next_start;

		while (line_start < text.size() && text[line_start] == ' ')
			line_start++;
	}

	return lines;
}

void rnd::Font::DrawRange(Canvas *canvas, const String &text, u64 start, u64 end, float x, float y) const
{
	Colour previous = canvas->GetColour();
	canvas->SetColour(colour);

	float pen_x = x;
	float line_top = y + cap_top * scale;

	if (integer_positions) {
		pen_x = std::round(pen_x);
		line_top = std::round(line_top);
	}

	for (u64 i = start; i < end && i < text.size(); i++) {
		unsigned c = static_cast<u8>(text[i]);

		const FontGlyph *g = GetGlyph(c);

		if (!g)
			continue;

		if (g->width > 0 && g->height > 0 && g->page < pages.size()) {
			TextureRegion region(pages[g->page].get(), g->x, g->y, g->width, g->height);

			float gx = pen_x + static_cast<float>(g->offset_x) * scale;
			float gy = line_top - static_cast<float>(g->offset_y + g->height) * scale;

			if (integer_positions) {
				gx = std::round(gx);
				gy = std::round(gy);
			}

			canvas->Draw(region, gx, gy, static_cast<float>(g->width) * scale, static_cast<float>(g->height) * scale);
		}

		pen_x += GetAdvanceAt(text, i);
	}

	canvas->SetColour(previous);
}
