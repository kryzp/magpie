#include "render/canvas.h"
#include "render/image.h"
#include "graphics/device.h"
#include "asset/asset_system.h"

#include <cmath>

rnd::TextureRegion::TextureRegion()
	: image(nullptr)
	, u(0.f)
	, v(0.f)
	, u2(1.f)
	, v2(1.f)
	, x(0)
	, y(0)
	, width(0)
	, height(0)
{
}

rnd::TextureRegion::TextureRegion(const Image *image_)
	: TextureRegion()
{
	image = image_;

	if (image) {
		width = (int)image->GetWidth();
		height = (int)image->GetHeight();
	}
}

rnd::TextureRegion::TextureRegion(const Image *image_, int x_, int y_, int width_, int height_)
	: image(image_)
	, x(x_)
	, y(y_)
	, width(width_)
	, height(height_)
{
	float inv_w = 1.f;
	float inv_h = 1.f;

	if (image) {
		inv_w = 1.f / (float)image->GetWidth();
		inv_h = 1.f / (float)image->GetHeight();
	}

	u = (float)x * inv_w;
	v = (float)y * inv_h;
	u2 = (float)(x + width) * inv_w;
	v2 = (float)(y + height) * inv_h;
}

rnd::TextureRegion rnd::TextureRegion::FromUV(const Image *image, float u, float v, float u2, float v2)
{
	TextureRegion region;
	region.image = image;
	region.u = u;
	region.v = v;
	region.u2 = u2;
	region.v2 = v2;

	if (image) {
		region.x = (int)std::round(u * (float)image->GetWidth());
		region.y = (int)std::round(v * (float)image->GetHeight());
		region.width = (int)std::round(std::abs(u2 - u) * (float)image->GetWidth());
		region.height = (int)std::round(std::abs(v2 - v) * (float)image->GetHeight());
	}

	return region;
}

bool rnd::TextureRegion::IsValid() const
{
	return image != nullptr;
}

Vector<rnd::TextureRegion> rnd::TextureRegion::Split(int tile_width, int tile_height) const
{
	Vector<TextureRegion> out;

	if (tile_width <= 0 || tile_height <= 0)
		return out;

	int rows = height / tile_height;
	int cols = width / tile_width;

	for (int r = 0; r < rows; r++) {
		for (int c = 0; c < cols; c++) {
			out.emplace_back(
				image,
				x + c * tile_width,
				y + r * tile_height,
				tile_width,
				tile_height
			);
		}
	}

	return out;
}

rnd::Canvas::Canvas(const Image *white_)
	: white(white_)
	, projection(1.f)
	, pixel_size(1.f)
	, has_transform(false)
	, transform(1.f)
	, colour(Colour::White())
	, blend(CanvasBlend_Alpha)
	, default_blend(CanvasBlend_Alpha)
{
}

void rnd::Canvas::Begin(const Mat4 &projection_, float pixel_size_)
{
	Begin(projection_, pixel_size_, CanvasBlend_Alpha);
}

void rnd::Canvas::Begin(const Mat4 &projection_, float pixel_size_, CanvasBlend default_blend_)
{
	default_blend = default_blend_;
	projection = projection_;
	pixel_size = pixel_size_;

	has_transform = false;
	transform = Mat4(1.f);

	colour = Colour::White();
	blend = default_blend;

	vertices.clear();
	batches.clear();
}

const Mat4 &rnd::Canvas::GetProjection() const
{
	return projection;
}

float rnd::Canvas::GetPixelSize() const
{
	return pixel_size;
}

void rnd::Canvas::SetTransform(const Mat4 &transform_)
{
	transform = transform_;
	has_transform = true;
}

void rnd::Canvas::ResetTransform()
{
	transform = Mat4(1.f);
	has_transform = false;
}

void rnd::Canvas::SetColour(const Colour &colour_)
{
	colour = colour_;
}

void rnd::Canvas::SetColour(float r, float g, float b, float a)
{
	colour = Colour(r, g, b, a);
}

const Colour &rnd::Canvas::GetColour() const
{
	return colour;
}

void rnd::Canvas::SetBlend(CanvasBlend blend_)
{
	blend = blend_;
}

rnd::CanvasBlend rnd::Canvas::GetBlend() const
{
	return blend;
}

void rnd::Canvas::Draw(const TextureRegion &region, float x, float y)
{
	Draw(region, x, y, (float)region.width, (float)region.height);
}

void rnd::Canvas::Draw(const TextureRegion &region, float x, float y, float width, float height)
{
	DrawFlipped(region, x, y, width, height, false, false);
}

void rnd::Canvas::Draw(
	const TextureRegion &region,
	float x, float y,
	float origin_x, float origin_y,
	float width, float height,
	float scale_x, float scale_y,
	float rotation
)
{
	if (!region.image)
		return;

	float world_origin_x = x + origin_x;
	float world_origin_y = y + origin_y;

	float fx = -origin_x * scale_x;
	float fy = -origin_y * scale_y;
	float fx2 = (width - origin_x) * scale_x;
	float fy2 = (height - origin_y) * scale_y;

	Vec2 corners[4] = {
		Vec2(fx, fy),
		Vec2(fx2, fy),
		Vec2(fx2, fy2),
		Vec2(fx, fy2)
	};

	if (rotation != 0.f) {
		float c = std::cos(rotation * maths::DEG_TO_RADf);
		float s = std::sin(rotation * maths::DEG_TO_RADf);

		for (Vec2 &p : corners) {
			float rx = c * p.x - s * p.y;
			float ry = s * p.x + c * p.y;
			p = Vec2(rx, ry);
		}
	}

	for (Vec2 &p : corners)
		p = Vec2(p.x + world_origin_x, p.y + world_origin_y);

	// v grows downwards in the image
	// y grows upwards on the canvas
	Vec2 uvs[4] = {
		Vec2(region.u, region.v2),
		Vec2(region.u2, region.v2),
		Vec2(region.u2, region.v),
		Vec2(region.u, region.v)
	};

	Colour colours[4] = { colour, colour, colour, colour };

	PushQuad(corners, uvs, colours, region.image->GetBindless());
}

void rnd::Canvas::DrawFlipped(const TextureRegion &region, float x, float y, float width, float height, bool flip_x, bool flip_y)
{
	if (!region.image)
		return;

	float u = region.u;
	float u2 = region.u2;
	float v = region.v;
	float v2 = region.v2;

	if (flip_x) {
		float t = u;
		u = u2;
		u2 = t;
	}

	if (flip_y) {
		float t = v;
		v = v2;
		v2 = t;
	}

	Vec2 corners[4] = {
		Vec2(x, y),
		Vec2(x + width, y),
		Vec2(x + width, y + height),
		Vec2(x, y + height)
	};

	Vec2 uvs[4] = {
		Vec2(u, v2),
		Vec2(u2, v2),
		Vec2(u2, v),
		Vec2(u, v)
	};

	Colour colours[4] = { colour, colour, colour, colour };

	PushQuad(corners, uvs, colours, region.image->GetBindless());
}

void rnd::Canvas::Quad(const Image *image, const Vec2 *positions, const Vec2 *uvs, const Colour *colours)
{
	const Image *used = image;

	if (!used)
		used = white;

	PushQuad(positions, uvs, colours, used->GetBindless());
}

void rnd::Canvas::FilledTriangle(const Vec2 &a, const Vec2 &b, const Vec2 &c)
{
	Vec2 p[3] = { a, b, c };
	Vec2 uv[3] = { Vec2(0.5f, 0.5f), Vec2(0.5f, 0.5f), Vec2(0.5f, 0.5f) };
	Colour col[3] = { colour, colour, colour };

	PushTriangle(p, uv, col, white->GetBindless());
}

void rnd::Canvas::FilledRect(float x, float y, float width, float height)
{
	Vec2 p[4] = {
		Vec2(x, y),
		Vec2(x + width, y),
		Vec2(x + width, y + height),
		Vec2(x, y + height)
	};

	Vec2 uv[4] = { Vec2(0.5f, 0.5f), Vec2(0.5f, 0.5f), Vec2(0.5f, 0.5f), Vec2(0.5f, 0.5f) };
	Colour col[4] = { colour, colour, colour, colour };

	PushQuad(p, uv, col, white->GetBindless());
}

void rnd::Canvas::FilledCircle(float cx, float cy, float radius, unsigned segments)
{
	FilledArc(cx, cy, radius, 0.f, 360.f, segments);
}

void rnd::Canvas::FilledArc(float cx, float cy, float radius, float start_degrees, float degrees, unsigned segments)
{
	if (segments == 0)
		return;

	float step = degrees * maths::DEG_TO_RADf / (float)segments;
	float angle = start_degrees * maths::DEG_TO_RADf;

	Vec2 centre(cx, cy);
	Vec2 prev(cx + std::cos(angle) * radius, cy + std::sin(angle) * radius);

	for (unsigned i = 0; i < segments; i++) {
		angle += step;

		Vec2 next(cx + std::cos(angle) * radius, cy + std::sin(angle) * radius);

		FilledTriangle(centre, prev, next);

		prev = next;
	}
}

void rnd::Canvas::Line(float x1, float y1, float x2, float y2)
{
	Line(x1, y1, x2, y2, pixel_size);
}

void rnd::Canvas::Line(float x1, float y1, float x2, float y2, float thickness)
{
	float dx = x2 - x1;
	float dy = y2 - y1;
	float length = std::sqrt(dx * dx + dy * dy);

	if (length <= 0.00001f) {
		FilledRect(x1 - thickness * 0.5f, y1 - thickness * 0.5f, thickness, thickness);
		return;
	}

	float nx = -dy / length * thickness * 0.5f;
	float ny = dx / length * thickness * 0.5f;

	Vec2 p[4] = {
		Vec2(x1 + nx, y1 + ny),
		Vec2(x1 - nx, y1 - ny),
		Vec2(x2 - nx, y2 - ny),
		Vec2(x2 + nx, y2 + ny)
	};

	Vec2 uv[4] = { Vec2(0.5f, 0.5f), Vec2(0.5f, 0.5f), Vec2(0.5f, 0.5f), Vec2(0.5f, 0.5f) };
	Colour col[4] = { colour, colour, colour, colour };

	PushQuad(p, uv, col, white->GetBindless());
}

void rnd::Canvas::Rect(float x, float y, float width, float height)
{
	Rect(x, y, width, height, pixel_size);
}

void rnd::Canvas::Rect(float x, float y, float width, float height, float thickness)
{
	FilledRect(x, y, width, thickness);
	FilledRect(x, y + height - thickness, width, thickness);
	FilledRect(x, y + thickness, thickness, height - thickness * 2.f);
	FilledRect(x + width - thickness, y + thickness, thickness, height - thickness * 2.f);
}

void rnd::Canvas::Circle(float cx, float cy, float radius, unsigned segments)
{
	Circle(cx, cy, radius, segments, pixel_size);
}

void rnd::Canvas::Circle(float cx, float cy, float radius, unsigned segments, float thickness)
{
	if (segments == 0)
		return;

	float step = maths::PI2f / (float)segments;

	for (unsigned i = 0; i < segments; i++) {
		float a0 = step * (float)i;
		float a1 = step * (float)(i + 1);

		Line(
			cx + std::cos(a0) * radius, cy + std::sin(a0) * radius,
			cx + std::cos(a1) * radius, cy + std::sin(a1) * radius,
			thickness
		);
	}
}

bool rnd::Canvas::IsEmpty() const
{
	return vertices.empty();
}

const Vector<rnd::CanvasVertex> &rnd::Canvas::GetVertices() const
{
	return vertices;
}

const Vector<rnd::CanvasBatch> &rnd::Canvas::GetBatches() const
{
	return batches;
}

Vec2 rnd::Canvas::Apply(const Vec2 &position) const
{
	if (!has_transform)
		return position;

	Vec3 transformed = transform.TransformPoint(Vec3(position.x, position.y, 0.f));

	return Vec2(transformed.x, transformed.y);
}

void rnd::Canvas::EnsureBatch()
{
	if (!batches.empty() && batches.back().blend == blend)
		return;

	CanvasBatch batch;
	batch.blend = blend;
	batch.first_vertex = vertices.size();
	batch.vertex_count = 0;

	batches.push_back(batch);
}

void rnd::Canvas::Push(const Vec2 &position, const Vec2 &uv, const Colour &c, unsigned texture)
{
	CanvasVertex v;
	v.position = Apply(position);
	v.uv = uv;
	v.colour = Vec4(c.r, c.g, c.b, c.a);
	v.texture = texture;
	v._pad0 = 0;
	v._pad1 = 0;
	v._pad2 = 0;

	vertices.push_back(v);
	batches.back().vertex_count++;
}

void rnd::Canvas::PushTriangle(const Vec2 *p, const Vec2 *uv, const Colour *c, unsigned texture)
{
	EnsureBatch();

	Push(p[0], uv[0], c[0], texture);
	Push(p[1], uv[1], c[1], texture);
	Push(p[2], uv[2], c[2], texture);
}

void rnd::Canvas::PushQuad(const Vec2 *p, const Vec2 *uv, const Colour *c, unsigned texture)
{
	EnsureBatch();

	Push(p[0], uv[0], c[0], texture);
	Push(p[1], uv[1], c[1], texture);
	Push(p[2], uv[2], c[2], texture);

	Push(p[0], uv[0], c[0], texture);
	Push(p[2], uv[2], c[2], texture);
	Push(p[3], uv[3], c[3], texture);
}

rnd::CanvasRenderer::CanvasRenderer()
{
	shader = ast::AssetSystem::Get()->RequireAssetBlocking("engine://shaders/canvas/canvas.slang");
}

bool rnd::CanvasRenderer::IsReady() const
{
	ast::AssetSystem *assets = ast::AssetSystem::Get();

	return assets->GetLoadState(shader) == ast::LoadState_Ready;
}

gfx::BlendState rnd::CanvasRenderer::BlendStateOf(CanvasBlend blend)
{
	switch (blend) {
		case CanvasBlend_Alpha: {
			return gfx::BlendState::Uniform(VK_BLEND_OP_ADD, VK_BLEND_FACTOR_SRC_ALPHA, VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA);
		} break;

		case CanvasBlend_Premultiplied: {
			gfx::BlendState state = gfx::BlendState::Uniform(VK_BLEND_OP_ADD, VK_BLEND_FACTOR_SRC_ALPHA, VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA);
			state.alpha.src = VK_BLEND_FACTOR_ONE;
			return state;
		} break;

		case CanvasBlend_Additive: {
			return gfx::BlendState::Uniform(VK_BLEND_OP_ADD, VK_BLEND_FACTOR_SRC_ALPHA, VK_BLEND_FACTOR_ONE);
		} break;

		case CanvasBlend_Opaque: {
		} break;
	}

	return gfx::BlendState::Disabled();
}

void rnd::CanvasRenderer::Record(
	const gfx::CmdBuffer &cmd,
	const gfx::RenderInfo &render_info,
	gfx::RingBuffer *upload,
	const Canvas &canvas,
	gfx::ResourceKey sampler
) const
{
	if (!IsReady())
		return;

	gfx::ResourceKey program = ast::AssetSystem::Get()->GetOrBreak(shader)->shader.key;

	Draw(cmd, render_info, upload, canvas, sampler, program, nullptr);
}

void rnd::CanvasRenderer::RecordWith(
	const gfx::CmdBuffer &cmd,
	const gfx::RenderInfo &render_info,
	gfx::RingBuffer *upload,
	const Canvas &canvas,
	gfx::ResourceKey sampler,
	gfx::ResourceKey program,
	const gfx::BlendState &blend
) const
{
	Draw(cmd, render_info, upload, canvas, sampler, program, &blend);
}

void rnd::CanvasRenderer::Draw(
	const gfx::CmdBuffer &cmd,
	const gfx::RenderInfo &render_info,
	gfx::RingBuffer *upload,
	const Canvas &canvas,
	gfx::ResourceKey sampler,
	gfx::ResourceKey program,
	const gfx::BlendState *blend_override
) const
{
	if (canvas.IsEmpty())
		return;

	gfx::Device *device = gfx::Device::Get();

	const Vector<CanvasVertex> &vertices = canvas.GetVertices();

	gfx::Alloc vertex_alloc = upload->PushCopyArray(vertices.data(), vertices.size());

	if (!vertex_alloc.cpu)
		return;

	struct {
		Mat4 projection;
		u64 vertices;
		u32 sampler;
		u32 pad;
	} args;

	args.projection = canvas.GetProjection();
	args.vertices = vertex_alloc.gpu;
	args.sampler = device->SamplerBindless(sampler);
	args.pad = 0;

	for (const CanvasBatch &batch : canvas.GetBatches()) {
		if (batch.vertex_count == 0)
			continue;

		gfx::GraphicsPipelineDef def(program, render_info);
		def.cull_mode = VK_CULL_MODE_NONE;

		if (blend_override)
			def.blend_state = *blend_override;
		else
			def.blend_state = BlendStateOf(batch.blend);

		gfx::PipelineState pipeline = device->FetchGraphicsPipeline(def);

		// SV_VertexID doesn't include the first vertex...
		// so the batch offset goes into the vertex pointer instead
		// :P
		args.vertices = vertex_alloc.gpu + (u64)batch.first_vertex * sizeof(CanvasVertex);

		cmd.BindPipelineState(pipeline);
		cmd.PushConstants(pipeline.layout, VK_SHADER_STAGE_ALL_GRAPHICS, args, 0);
		cmd.Draw(batch.vertex_count, 1, 0, 0);
	}
}
