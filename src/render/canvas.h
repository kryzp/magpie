#ifndef RENDER_CANVAS_H
#define RENDER_CANVAS_H

#include "core/types.h"
#include "core/mat4.h"
#include "core/vec2.h"
#include "core/colour.h"
#include "graphics/resource_key.h"
#include "graphics/ring_buffer.h"
#include "graphics/cmd_buffer.h"
#include "graphics/pipeline.h"
#include "asset/handle.h"

#include "container/vector.h"

namespace rnd {

class Image;

struct TextureRegion {
	const Image *image;
	float u, v;
	float u2, v2;
	int x, y;
	int width, height;

	TextureRegion();
	explicit TextureRegion(const Image *image);
	TextureRegion(const Image *image, int x, int y, int width, int height);

	static TextureRegion FromUV(const Image *image, float u, float v, float u2, float v2);

	[[nodiscard]] bool IsValid() const;

	[[nodiscard]] Vector<TextureRegion> Split(int tile_width, int tile_height) const;
};

enum CanvasBlend {
	CanvasBlend_Alpha,         // src alpha, 1 - src alpha
	CanvasBlend_Premultiplied, // src alpha, alpha accumulates as one, 1 - src alpha
	CanvasBlend_Additive,      // src alpha, one
	CanvasBlend_Opaque         // no blending
};

struct CanvasVertex {
	Vec2 position;
	Vec2 uv;
	Vec4 colour;
	u32 texture;
	u32 _pad0;
	u32 _pad1;
	u32 _pad2;
};

struct CanvasBatch {
	CanvasBlend blend;
	unsigned first_vertex;
	unsigned vertex_count;
};

class Canvas {
public:
	explicit Canvas(const Image *white);

	// pixel_size == how many canvas units cover one target pixels
	// outlines also default to that as thickness (1 pixel_size unit)
	void Begin(const Mat4 &projection, float pixel_size);
	void Begin(const Mat4 &projection, float pixel_size, CanvasBlend default_blend);

	[[nodiscard]] const Mat4 &GetProjection() const;
	[[nodiscard]] float GetPixelSize() const;

	void SetTransform(const Mat4 &transform);
	void ResetTransform();

	void SetColour(const Colour &colour);
	void SetColour(float r, float g, float b, float a);
	[[nodiscard]] const Colour &GetColour() const;

	void SetBlend(CanvasBlend blend);
	[[nodiscard]] CanvasBlend GetBlend() const;

	void Draw(const TextureRegion &region, float x, float y);
	void Draw(const TextureRegion &region, float x, float y, float width, float height);

	void Draw(
		const TextureRegion &region,
		float x, float y,
		float origin_x, float origin_y,
		float width, float height,
		float scale_x, float scale_y,
		float rotation
	);

	void DrawFlipped(const TextureRegion &region, float x, float y, float width, float height, bool flip_x, bool flip_y);

	// corners counter-clockwise from the bottom-left
	void Quad(const Image *image, const Vec2 *positions, const Vec2 *uvs, const Colour *colours);

	void FilledTriangle(const Vec2 &a, const Vec2 &b, const Vec2 &c);
	void FilledRect(float x, float y, float width, float height);
	void FilledCircle(float cx, float cy, float radius, unsigned segments);

	void FilledArc(float cx, float cy, float radius, float start_degrees, float degrees, unsigned segments);

	void Line(float x1, float y1, float x2, float y2);
	void Line(float x1, float y1, float x2, float y2, float thickness);
	void Rect(float x, float y, float width, float height);
	void Rect(float x, float y, float width, float height, float thickness);
	void Circle(float cx, float cy, float radius, unsigned segments);
	void Circle(float cx, float cy, float radius, unsigned segments, float thickness);

	[[nodiscard]] bool IsEmpty() const;

	[[nodiscard]] const Vector<CanvasVertex> &GetVertices() const;
	[[nodiscard]] const Vector<CanvasBatch> &GetBatches() const;

private:
	void Push(const Vec2 &position, const Vec2 &uv, const Colour &colour, unsigned texture);
	void PushTriangle(const Vec2 *p, const Vec2 *uv, const Colour *c, unsigned texture);
	void PushQuad(const Vec2 *p, const Vec2 *uv, const Colour *c, unsigned texture);
	void EnsureBatch();

	[[nodiscard]] Vec2 Apply(const Vec2 &position) const;

	const Image *white;

	Mat4 projection;
	float pixel_size;

	bool has_transform;
	Mat4 transform;

	Colour colour;
	CanvasBlend blend;
	CanvasBlend default_blend;

	Vector<CanvasVertex> vertices;
	Vector<CanvasBatch> batches;
};

class CanvasRenderer {
public:
	CanvasRenderer();

	[[nodiscard]] bool IsReady() const;

	void Record(
		const gfx::CmdBuffer &cmd,
		const gfx::RenderInfo &render_info,
		gfx::RingBuffer *upload,
		const Canvas &canvas,
		gfx::ResourceKey sampler
	) const;

	void RecordWith(
		const gfx::CmdBuffer &cmd,
		const gfx::RenderInfo &render_info,
		gfx::RingBuffer *upload,
		const Canvas &canvas,
		gfx::ResourceKey sampler,
		gfx::ResourceKey program,
		const gfx::BlendState &blend
	) const;

	static gfx::BlendState BlendStateOf(CanvasBlend blend);

private:
	void Draw(
		const gfx::CmdBuffer &cmd,
		const gfx::RenderInfo &render_info,
		gfx::RingBuffer *upload,
		const Canvas &canvas,
		gfx::ResourceKey sampler,
		gfx::ResourceKey program,
		const gfx::BlendState *blend_override
	) const;

	ast::Handle shader;
};

}

#endif // RENDER_CANVAS_H
