#include "render/debug/debug_renderer.h"
#include "render/graph/graph.h"
#include "graphics/device.h"

#include <cmath>
#include <algorithm>
#include <string.h>

static rnd::DebugRenderer *selected_debug_renderer = nullptr;

static void UploadDebugMesh(rnd::Mesh *mesh, const Vec3 *vertices, unsigned vertex_count, const unsigned short *indices, unsigned index_count)
{
	mesh->Alloc(sizeof(Vec3), gfx::IndexType_U16, vertex_count, index_count);
	mesh->UploadBlocking(vertices, indices);
}

static void CreateLineMesh(rnd::Mesh *mesh)
{
	static const Vec3 vertices[] = {
		Vec3(0.f, 0.f, 1.f),
		Vec3(0.f, 0.f, 0.f),
	};

	static const unsigned short indices[] = { 0, 1 };

	UploadDebugMesh(mesh, vertices, ArraySize(vertices), indices, ArraySize(indices));
}

static void CreateCrossMesh(rnd::Mesh *mesh)
{
	static const Vec3 vertices[] = {
		Vec3(1.f, 1.f, 1.f),
		Vec3(-1.f, -1.f, -1.f),
		Vec3(-1.f, 1.f, 1.f),
		Vec3(1.f, -1.f, -1.f),
		Vec3(1.f, -1.f, 1.f),
		Vec3(-1.f, 1.f, -1.f),
		Vec3(-1.f, -1.f, 1.f),
		Vec3(1.f, 1.f, -1.f),
	};

	static const unsigned short indices[] = {
		0, 1,
		2, 3,
		4, 5,
		6, 7
	};

	UploadDebugMesh(mesh, vertices, ArraySize(vertices), indices, ArraySize(indices));
}

static void CreateSphereMesh(rnd::Mesh *mesh)
{
	const unsigned segments = 8;
	const unsigned rings = 4;

	const unsigned vertex_count = (rings + 1) * (segments + 1);
	const unsigned index_count = rings * segments * 6;

	Vector<Vec3> vertices(vertex_count);
	Vector<unsigned short> indices(index_count);

	unsigned v_idx = 0;
	unsigned i_idx = 0;

	for (unsigned ring = 0; ring <= rings; ring++) {
		float v = (float)ring / (float)rings;
		for (unsigned segment = 0; segment <= segments; segment++) {
			float u = (float)segment / (float)segments;

			float phi = (v - 0.5f) * maths::PIf;
			float theta = u * maths::PIf * 2.f;

			vertices[v_idx++] = Vec3::SphericalToCartesian(1.f, phi, theta);
		}
	}

	for (unsigned ring = 0; ring < rings; ring++) {
		for (unsigned segment = 0; segment < segments; segment++) {

			unsigned a = ring * (segments + 1) + segment;
			unsigned b = a + 1;
			unsigned c = a + (segments + 1);
			unsigned d = c + 1;

			indices[i_idx++] = a;
			indices[i_idx++] = c;
			indices[i_idx++] = b;

			indices[i_idx++] = b;
			indices[i_idx++] = c;
			indices[i_idx++] = d;
		}
	}

	UploadDebugMesh(mesh, vertices.data(), vertex_count, indices.data(), index_count);
}

static void CreateCircleMesh(rnd::Mesh *mesh)
{
	const unsigned segments = 32;

	unsigned vertex_count = segments;
	unsigned index_count = segments * 2;

	Vector<Vec3> vertices(vertex_count);
	Vector<unsigned short> indices(index_count);

	for (unsigned i = 0; i < segments; i++) {
		float phi = ((float)i / (float)segments) * (maths::PIf * 2.f);
		vertices[i] = Vec3::SphericalToCartesian(1.f, phi, 0.f);
		indices[i * 2 + 0] = (i + 0) % segments;
		indices[i * 2 + 1] = (i + 1) % segments;
	}

	UploadDebugMesh(mesh, vertices.data(), vertex_count, indices.data(), index_count);
}

static void CreateCubeMesh(rnd::Mesh *mesh)
{
	static const Vec3 vertices[] = {
		Vec3(-1.f, -1.f, -1.f),
		Vec3(1.f, -1.f, -1.f),
		Vec3(1.f, 1.f, -1.f),
		Vec3(-1.f, 1.f, -1.f),
		Vec3(-1.f, -1.f, 1.f),
		Vec3(1.f, -1.f, 1.f),
		Vec3(1.f, 1.f, 1.f),
		Vec3(-1.f, 1.f, 1.f),
	};

	static const unsigned short indices[] = {
		0,1,1,
		2,2,3,

		3,0,4,
		5,5,6,

		6,7,7,
		4,0,4,

		1,5,2,
		6,3,7,
	};

	UploadDebugMesh(mesh, vertices, ArraySize(vertices), indices, ArraySize(indices));
}

rnd::DebugRenderer::DebugRenderer()
{
	CreateLineMesh(&line_mesh);
	CreateCrossMesh(&cross_mesh);
	CreateSphereMesh(&sphere_mesh);
	CreateCircleMesh(&circle_mesh);
	CreateCubeMesh(&cube_mesh);

	Select(this);
}

rnd::DebugRenderer::~DebugRenderer()
{
	line_mesh.Destroy();
	cross_mesh.Destroy();
	sphere_mesh.Destroy();
	circle_mesh.Destroy();
	cube_mesh.Destroy();

	if (selected_debug_renderer == this)
		selected_debug_renderer = nullptr;
}

rnd::DebugRenderer *rnd::DebugRenderer::Get()
{
	return selected_debug_renderer;
}

void rnd::DebugRenderer::Select(DebugRenderer *renderer)
{
	selected_debug_renderer = renderer;
}

void rnd::DebugRenderer::PushDrawCall(DebugDrawType type, const DebugDrawCall &call, bool depth_enabled_)
{
	SpinLockScope lock(spinlock);

	Bucket *bucket = depth_enabled_
		 ? &depth_enabled[type]
		 : &depth_disabled[type];

	bucket->push_back(call);
}

void rnd::DebugRenderer::PushLine(
	const Vec3 &from, const Vec3 &to,
	const Vec4 &colour, float line_width,
	float duration, bool depth_enabled_
)
{
	DebugDrawCall call;
	call.colour = colour;
	call.line_width = line_width;
	call.duration = duration;
	call.initial_duration = duration;
	call.a = from;
	call.b = to;

	PushDrawCall(DebugDrawType_Line, call, depth_enabled_);
}

void rnd::DebugRenderer::PushCross(
	const Vec3 &point, float size,
	const Vec4 &colour, float duration, bool depth_enabled_
)
{
	DebugDrawCall call;
	call.colour = colour;
	call.line_width = 1.f;
	call.duration = duration;
	call.initial_duration = duration;
	call.a = point;
	call.size = size;

	PushDrawCall(DebugDrawType_Cross, call, depth_enabled_);
}

void rnd::DebugRenderer::PushSphere(
	const Vec3 &centre, float radius,
	const Vec4 &colour, float duration, bool depth_enabled_
)
{
	DebugDrawCall call;
	call.colour = colour;
	call.line_width = 1.f;
	call.duration = duration;
	call.initial_duration = duration;
	call.a = centre;
	call.size = radius;

	PushDrawCall(DebugDrawType_Sphere, call, depth_enabled_);
}

void rnd::DebugRenderer::PushCircle(
	const Vec3 &centre, float radius, const Vec3 &plane_normal,
	const Vec4 &colour, float duration, bool depth_enabled_
)
{
	DebugDrawCall call;
	call.colour = colour;
	call.line_width = 1.f;
	call.duration = duration;
	call.initial_duration = duration;
	call.a = centre;
	call.size = radius;
	call.b = plane_normal;

	PushDrawCall(DebugDrawType_Circle, call, depth_enabled_);
}

void rnd::DebugRenderer::PushTriangle(
	const Vec3 &a, const Vec3 &b, const Vec3 &c,
	const Vec4 &colour, float line_width,
	float duration, bool depth_enabled_
)
{
	DebugDrawCall call;
	call.colour = colour;
	call.line_width = line_width;
	call.duration = duration;
	call.initial_duration = duration;
	call.a = a;
	call.b = b;
	call.c = c;

	PushDrawCall(DebugDrawType_Triangle, call, depth_enabled_);
}

void rnd::DebugRenderer::PushAABB(
	const Vec3 &min, const Vec3 &max,
	const Vec4 &colour, float line_width,
	float duration, bool depth_enabled_
)
{
	DebugDrawCall call;
	call.colour = colour;
	call.line_width = line_width;
	call.duration = duration;
	call.initial_duration = duration;
	call.a = min;
	call.b = max;

	PushDrawCall(DebugDrawType_AABB, call, depth_enabled_);
}

void rnd::DebugRenderer::PushOBB(
	const Mat4 &transform, const Vec3 &scale,
	const Vec4 &colour, float line_width,
	float duration, bool depth_enabled_
)
{
	DebugDrawCall call;
	call.colour = colour;
	call.line_width = line_width;
	call.duration = duration;
	call.initial_duration = duration;
	call.transform = transform;
	call.b = scale;

	PushDrawCall(DebugDrawType_OBB, call, depth_enabled_);
}

static void WriteInstance(
	Vector<rnd::GPU_DebugObjectDraw> *draws,
	const Mat4 &transform, const Vec4 &colour, float thickness, float alpha
)
{
	if (draws->size() >= RENDER_DEBUG_MAX_DRAWS)
		return;

	Vec4 premul_colour = Vec4(
		colour.x * colour.w,
		colour.y * colour.w,
		colour.z * colour.w,
		colour.w * alpha
	);

	rnd::GPU_DebugObjectDraw draw;
	draw.transform = transform;
	draw.colour = premul_colour;
	draw.thickness = thickness;

	draws->push_back(draw);
}

static float CallAlpha(const rnd::DebugDrawCall &call)
{
	if (call.initial_duration <= maths::EPSILONf)
		return 1.f;

	float t = call.duration / call.initial_duration;

	return maths::Clamp(t, 0.f, 1.f);
}

static void BuildLineInstance(
	Vector<rnd::GPU_DebugObjectDraw> *draws,
	const Vec3 &from, const Vec3 &to, const Vec4 &colour, float thickness, float alpha
)
{
	//Vec3 direction = (to - from).Normalized();
	float length = (to - from).Length();

	Mat4 transform = Mat4::Translate(from) * Mat4::Scale(Vec3(length, length, length));
	// TODO: Mat4::RotateAround for the direction vector.

	WriteInstance(draws, transform, colour, thickness, alpha);
}

void rnd::DebugRenderer::BuildInstances(const Bucket *buckets, Vector<GPU_DebugObjectDraw> *draws)
{
	for (unsigned type = 0; type < DebugDrawType_MaxEnum; type++) {
		for (const DebugDrawCall &call : buckets[type]) {
			float alpha = CallAlpha(call);

			switch ((DebugDrawType)type) {
				case DebugDrawType_Line: {
					BuildLineInstance(draws, call.a, call.b, call.colour, call.line_width, alpha);
				} break;

				case DebugDrawType_Cross: {
					float half = call.size * 0.5f;
					Mat4 t = Mat4::Translate(call.a) * Mat4::Scale(Vec3(half, half, half));
					WriteInstance(draws, t, call.colour, call.line_width, alpha);
				} break;

				case DebugDrawType_Sphere: {
					float r = call.size;
					Mat4 t = Mat4::Translate(call.a) * Mat4::Scale(Vec3(r, r, r));
					WriteInstance(draws, t, call.colour, call.line_width, alpha);
				} break;

				case DebugDrawType_Circle: {
					float r = call.size;
					// TODO: Mat4::RotateAround for the normal vector.
					Mat4 t = Mat4::Translate(call.a) * Mat4::Scale(Vec3(r, r, r));
					WriteInstance(draws, t, call.colour, call.line_width, alpha);
				} break;

				case DebugDrawType_Triangle: {
					BuildLineInstance(draws, call.a, call.b, call.colour, call.line_width, alpha);
					BuildLineInstance(draws, call.b, call.c, call.colour, call.line_width, alpha);
					BuildLineInstance(draws, call.c, call.a, call.colour, call.line_width, alpha);
				} break;

				case DebugDrawType_AABB: {
					Vec3 centre = (call.b + call.a) * 0.5f;
					Vec3 size = call.b - call.a;
					Mat4 t = Mat4::Translate(centre) * Mat4::Scale(size);
					WriteInstance(draws, t, call.colour, call.line_width, alpha);
				} break;

				case DebugDrawType_OBB: {
					Mat4 t = call.transform * Mat4::Scale(call.b);
					WriteInstance(draws, t, call.colour, call.line_width, alpha);
				} break;

				default:
					break;
			}
		}
	}
}

unsigned rnd::DebugRenderer::BuildBatches(const Bucket *buckets, DebugBatch *out_batches, unsigned *running_id)
{
	unsigned batch_count = 0;

	for (unsigned type = 0; type < DebugDrawType_MaxEnum; type++) {
		unsigned count = 0;

		for (unsigned i = 0; i < buckets[type].size(); i++) {
			if ((DebugDrawType)type == DebugDrawType_Triangle)
				count += 3; // triangles produce 3 line instances
			else
				count += 1;
		}

		if (count == 0)
			continue;

		AssertTrue(batch_count < RENDER_DEBUG_MAX_BATCHES);

		DebugBatch *batch = &out_batches[batch_count++];
		batch->type = type;
		batch->start = *running_id;
		batch->count = count;

		*running_id += count;
	}

	return batch_count;
}

struct DebugPassData {
	const rnd::FrameParams *frame_params = nullptr;
	const rnd::DebugRenderer *renderer = nullptr;

	// This frame's instance arrays, in the frame's upload slice.
	u64 instance_address[rnd::DebugDepthMode_MaxEnum];
};

const rnd::Mesh *rnd::DebugRenderer::MeshForType(DebugDrawType type) const
{
	switch (type) {
		case DebugDrawType_Line:
			return &line_mesh;

		case DebugDrawType_Cross:
			return &cross_mesh;

		case DebugDrawType_Sphere:
			return &sphere_mesh;

		case DebugDrawType_Circle:
			return &circle_mesh;

		case DebugDrawType_AABB:
			return &cube_mesh;

		case DebugDrawType_OBB:
			return &cube_mesh;

		case DebugDrawType_Triangle:
			return &line_mesh;

		default:
			return &line_mesh;
	}
}

static void DrawBatches(
	const rnd::DebugRenderer *renderer,
	const gfx::CmdBuffer &cmd,
	const gfx::PipelineState &pipeline_st,
	const rnd::DebugBatch *batches,
	unsigned batch_count,
	u64 instance_address,
	const Mat4 &view_proj
)
{
	gfx::Device *device = gfx::Device::Get();

	cmd.BindPipeline(pipeline_st.bind_point, pipeline_st.pipeline);

	for (unsigned i = 0; i < batch_count; i++) {
		const rnd::DebugBatch *batch = &batches[i];
		const rnd::Mesh *mesh = renderer->MeshForType((rnd::DebugDrawType)batch->type);

		struct {
			Mat4 view_proj;
			u64 calls_buffer;
			u64 vertex_buffer;
		} args;

		args.view_proj = view_proj;
		args.calls_buffer = instance_address;
		args.vertex_buffer = device->BufferAddress(mesh->vertex_buffer);

		cmd.PushConstants(pipeline_st.layout, VK_SHADER_STAGE_ALL_GRAPHICS, args, 0);

		mesh->RecordBindIndexBuffer(cmd);

		cmd.DrawIndexed(mesh->index_count, batch->count, 0, 0, batch->start);
	}
}

static void DebugRenderPassFn(const rnd::PassContext &ctx)
{
	const gfx::CmdBuffer &cmd = *ctx.cmd;
	const DebugPassData *data = (const DebugPassData *)ctx.user_data;
	const rnd::FrameParams *frame_params = data->frame_params;

	gfx::Device *device = gfx::Device::Get();

	gfx::GraphicsPipelineDef pipeline_def(frame_params->debug_line_shader, *ctx.render_info);
	pipeline_def.topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
	pipeline_def.cull_mode = VK_CULL_MODE_NONE;
	pipeline_def.depth_stencil_state.depth_test_enabled = true;
	pipeline_def.depth_stencil_state.depth_write_enabled = false;
	pipeline_def.blend_state.enabled = true;
	pipeline_def.blend_state.colour.src = VK_BLEND_FACTOR_SRC_ALPHA;
	pipeline_def.blend_state.colour.dst = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	pipeline_def.blend_state.colour.op = VK_BLEND_OP_ADD;
	pipeline_def.blend_state.alpha.src = VK_BLEND_FACTOR_ONE;
	pipeline_def.blend_state.alpha.dst = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	pipeline_def.blend_state.alpha.op = VK_BLEND_OP_ADD;

	gfx::PipelineState pipeline_st = device->FetchGraphicsPipeline(pipeline_def);

	pipeline_def.depth_stencil_state.depth_test_enabled = false;
	gfx::PipelineState pipeline_st_no_depth = device->FetchGraphicsPipeline(pipeline_def);

	const Mat4 &view_proj = frame_params->camera.view_proj;
	const rnd::DebugPacket *debug = &frame_params->packet->debug;

	const unsigned tested = rnd::DebugDepthMode_Tested;
	const unsigned overlay = rnd::DebugDepthMode_Overlay;

	DrawBatches(
		data->renderer, cmd, pipeline_st,
		debug->batches[tested],
		debug->batch_count[tested],
		data->instance_address[tested],
		view_proj
	);

	DrawBatches(
		data->renderer, cmd, pipeline_st_no_depth,
		debug->batches[overlay],
		debug->batch_count[overlay],
		data->instance_address[overlay],
		view_proj
	);
}

void rnd::DebugRenderer::FilterBuckets(Bucket *buckets, float dt)
{
	for (unsigned type = 0; type < DebugDrawType_MaxEnum; type++) {
		Bucket &bucket = buckets[type];

		bucket.erase(std::remove_if(bucket.begin(), bucket.end(),
			[](const DebugDrawCall &call) -> bool {
				return call.duration < 0.f;
			}),
			bucket.end()
		);

		for (DebugDrawCall &call : bucket) {
			call.duration -= dt;
		}
	}
}

unsigned rnd::DebugRenderer::CountInstances(const Bucket *buckets)
{
	unsigned count = 0;

	for (unsigned type = 0; type < DebugDrawType_MaxEnum; type++)
		count += buckets[type].size() * ((type == DebugDrawType_Triangle) ? 3 : 1);

	return maths::Min<unsigned>(count, RENDER_DEBUG_MAX_DRAWS);
}

void rnd::DebugRenderer::CollectInto(FramePacket *packet, float dt)
{
	DebugPacket *out = &packet->debug;

	SpinLockScope lock(spinlock);

	// Expire old draws.
	FilterBuckets(depth_enabled, dt);
	FilterBuckets(depth_disabled, dt);

	const Bucket *buckets[DebugDepthMode_MaxEnum] = {
		depth_enabled,
		depth_disabled,
	};

	for (unsigned mode = 0; mode < DebugDepthMode_MaxEnum; mode++) {
		unsigned count = CountInstances(buckets[mode]);

		out->instances[mode].clear();
		out->instances[mode].reserve(count);

		BuildInstances(buckets[mode], &out->instances[mode]);

		unsigned instance_count = out->instances[mode].size();

		unsigned running_id = 0;
		out->batch_count[mode] = BuildBatches(buckets[mode], out->batches[mode], &running_id);

		// Keep batches inside what was actually written if we hit the cap.
		for (unsigned b = 0; b < out->batch_count[mode]; b++) {
			DebugBatch *batch = &out->batches[mode][b];
			unsigned end = maths::Min(batch->start + batch->count, instance_count);
			batch->count = (end > batch->start) ? (end - batch->start) : 0;
		}
	}
}

void rnd::DebugRenderer::Render(
	Graph *graph,
	const FrameParams *frame_params,
	GraphTexHandle target_colour,
	GraphTexHandle target_depth
)
{
	const DebugPacket *debug = &frame_params->packet->debug;

	if (debug->instances[DebugDepthMode_Tested].empty() &&
		debug->instances[DebugDepthMode_Overlay].empty()) {
		return;
	}

	DebugPassData *data = graph->NewPassData<DebugPassData>();
	data->frame_params = frame_params;
	data->renderer = this;

	for (unsigned mode = 0; mode < DebugDepthMode_MaxEnum; mode++) {
		const Vector<GPU_DebugObjectDraw> &instances = debug->instances[mode];

		gfx::Alloc alloc = frame_params->upload->Push(
			sizeof(GPU_DebugObjectDraw) * maths::Max<u64>(instances.size(), 1),
			alignof(GPU_DebugObjectDraw)
		);

		if (alloc.cpu && !instances.empty())
			memcpy(alloc.cpu, instances.data(), instances.size() * sizeof(GPU_DebugObjectDraw));

		data->instance_address[mode] = alloc.gpu;
	}

	Pass *pass = graph->Add("Debug Lines", PassType_Graphics);
	pass->SetRecord(DebugRenderPassFn, data);
	pass->WriteColour(target_colour, nullptr);
	pass->WriteDepth(target_depth, nullptr);
}
