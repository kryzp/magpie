#include "debug_renderer.h"

#include "core/scratch.h"
#include "assets/shader_serializer.h"
#include "math/calc.h"

#include "../camera.h"

using namespace gfx;

DebugRenderer::DebugRenderer()
	: device(nullptr)
	, depth_enabled_buckets{}
	, depth_disabled_buckets{}
	, shader_asset()
	, depth_enabled_call_buffer(nullptr)
	, depth_enabled_id(0)
	, depth_disabled_call_buffer(nullptr)
	, depth_disabled_id(0)
	, line_mesh()
	, cross_mesh()
	, sphere_mesh()
	, circle_mesh()
	, cube_mesh()
	, current_depth_enabled(false)
	, current_colour()
	, current_thickness()
	, current_alpha(0.f)
{
}

DebugRenderer::~DebugRenderer()
{
}

DebugRenderer *DebugRenderer::get_singleton()
{
	static DebugRenderer instance;
	return &instance;
}

struct GPU_DebugObjectDraw {
	Mat4 transform;
	Vec4 colour;
	float thickness;
};

void DebugRenderer::init(Device *device, ast::AssetManager &assets)
{
	this->device = device;
	this->assets = &assets;

	this->shader_asset = assets.from_file_path("assets://shaders/passes/debug_rendering.slang");

	BufferAllocInfo buffer_info = {};
	buffer_info.usage = VK_BUFFER_USAGE_2_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_2_TRANSFER_DST_BIT;
	buffer_info.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
	buffer_info.size = sizeof(GPU_DebugObjectDraw) * MAX_DEBUG_DRAWS;

	this->depth_enabled_call_buffer = device->alloc_buffer(buffer_info);
	this->depth_disabled_call_buffer = device->alloc_buffer(buffer_info);

	create_line_mesh();
	create_cross_mesh();
	create_circle_mesh();
	create_sphere_mesh();
	create_cube_mesh();
}

void DebugRenderer::destroy()
{
	line_mesh.destroy_buffers();
	cross_mesh.destroy_buffers();
	circle_mesh.destroy_buffers();
	sphere_mesh.destroy_buffers();
	cube_mesh.destroy_buffers();

	device->destroy_buffer(depth_enabled_call_buffer);
	device->destroy_buffer(depth_disabled_call_buffer);
}

void DebugRenderer::create_line_mesh()
{
	ScratchScope scratch = scratch::get();

	Vec3 *vertices = scratch.arena().array<Vec3>(2);
	IndexType *indices = scratch.arena().array<IndexType>(2);

	vertices[0] = Vec3(0.f, 0.f, 1.f);
	vertices[1] = Vec3(0.f, 0.f, 0.f);

	indices[0] = 0;
	indices[1] = 1;

	line_mesh.create_buffers(device, sizeof(Vec3), 2, 2);

	GpuBuffer *staging = device->alloc_stage(line_mesh.get_vertex_buffer_size() + line_mesh.get_index_buffer_size());

	line_mesh.write_to_staging_buffer(staging, 0, vertices, indices);

	device->submit_graphics_immediate([&](CommandBuffer &cmd) {
		line_mesh.batch_upload(cmd, staging, 0);
	});

	device->destroy_buffer(staging);
}

void DebugRenderer::create_cross_mesh()
{
	ScratchScope scratch = scratch::get();

	Vec3 *vertices = scratch.arena().array<Vec3>(8);
	IndexType *indices = scratch.arena().array<IndexType>(8);

	vertices[0] = Vec3( 1.f,  1.f,  1.f);
	vertices[1] = Vec3(-1.f, -1.f, -1.f);
	vertices[2] = Vec3(-1.f,  1.f,  1.f);
	vertices[3] = Vec3( 1.f, -1.f, -1.f);
	vertices[4] = Vec3( 1.f, -1.f,  1.f);
	vertices[5] = Vec3(-1.f,  1.f, -1.f);
	vertices[6] = Vec3(-1.f, -1.f,  1.f);
	vertices[7] = Vec3( 1.f,  1.f, -1.f);

	for (int i = 0; i < 8; i++)
		indices[i] = i;

	cross_mesh.create_buffers(device, sizeof(Vec3), 8, 8);

	GpuBuffer *staging = device->alloc_stage(cross_mesh.get_vertex_buffer_size() + cross_mesh.get_index_buffer_size());

	cross_mesh.write_to_staging_buffer(staging, 0, vertices, indices);

	device->submit_graphics_immediate([&](CommandBuffer &cmd) {
		cross_mesh.batch_upload(cmd, staging, 0);
	});

	device->destroy_buffer(staging);
}

void DebugRenderer::create_sphere_mesh()
{
	ScratchScope scratch = scratch::get();

	u32 segments = 32;
	u32 vertex_count = segments * 3;
	u32 index_count = segments * 6;

	Vec3 *vertices = scratch.arena().array<Vec3>(vertex_count);
	IndexType *indices = scratch.arena().array<IndexType>(index_count);

	u32 v_idx = 0;
	u32 i_idx = 0;

	for (u32 ring = 0; ring < 3; ring++) {
		u32 ring_start = v_idx;
		for (u32 i = 0; i < segments; i++) {
			float angle = ((float)i / segments) * CalcF::TAU;

			float c = CalcF::cos(angle);
			float s = CalcF::sin(angle);

			if (ring == 0) vertices[v_idx] = Vec3(c, s, 0.f);
			if (ring == 1) vertices[v_idx] = Vec3(c, 0.f, s);
			if (ring == 2) vertices[v_idx] = Vec3(0.f, c, s);

			indices[i_idx++] = v_idx;
			indices[i_idx++] = ring_start + ((i + 1) % segments);

			v_idx++;
		}
	}

	sphere_mesh.create_buffers(device, sizeof(Vec3), vertex_count, index_count);

	GpuBuffer *staging = device->alloc_stage(sphere_mesh.get_vertex_buffer_size() + sphere_mesh.get_index_buffer_size());

	sphere_mesh.write_to_staging_buffer(staging, 0, vertices, indices);

	device->submit_graphics_immediate([&](CommandBuffer &cmd) {
		sphere_mesh.batch_upload(cmd, staging, 0);
	});

	device->destroy_buffer(staging);
}

void DebugRenderer::create_circle_mesh()
{
	ScratchScope scratch = scratch::get();

	u32 segments = 32;
	
	u32 vertex_count = segments;
	u32 index_count  = segments * 2;
	
	Vec3      *vertices = scratch.arena().array<Vec3>(vertex_count);
	IndexType *indices  = scratch.arena().array<IndexType>(index_count);
	
	for (u32 i = 0; i < segments; i++) {
		float phi = ((float)i / segments) * CalcF::TAU;
		
		vertices[i] = Vec3::spherical_to_cartesian(1.f, phi, 0.f);
		
		indices[i * 2] = i;
		indices[i * 2 + 1] = (i + 1) % segments; 
	}

	circle_mesh.create_buffers(device, sizeof(Vec3), vertex_count, index_count);
	
	GpuBuffer *staging = device->alloc_stage(circle_mesh.get_vertex_buffer_size() + circle_mesh.get_index_buffer_size());
	
	circle_mesh.write_to_staging_buffer(staging, 0, vertices, indices);
	
	device->submit_graphics_immediate([&](CommandBuffer &cmd) {
		circle_mesh.batch_upload(cmd, staging, 0);
	});

	device->destroy_buffer(staging);
}

void DebugRenderer::create_cube_mesh()
{
	ScratchScope scratch = scratch::get();

	u32 vertex_count = 8;
	u32 index_count = 24;

	Vec3 *vertices = scratch.arena().array<Vec3>(vertex_count);
	IndexType *indices = scratch.arena().array<IndexType>(index_count);

	vertices[0] = Vec3(-1.f, -1.f, -1.f);
	vertices[1] = Vec3( 1.f, -1.f, -1.f);
	vertices[2] = Vec3( 1.f,  1.f, -1.f);
	vertices[3] = Vec3(-1.f,  1.f, -1.f);
	vertices[4] = Vec3(-1.f, -1.f,  1.f);
	vertices[5] = Vec3( 1.f, -1.f,  1.f);
	vertices[6] = Vec3( 1.f,  1.f,  1.f);
	vertices[7] = Vec3(-1.f,  1.f,  1.f);

	IndexType line_indices[] = {
		0, 1, 1, 2, 2, 3, 3, 0,
		4, 5, 5, 6, 6, 7, 7, 4,
		0, 4, 1, 5, 2, 6, 3, 7
	};

	memcpy(indices, line_indices, sizeof(line_indices));

	cube_mesh.create_buffers(device, sizeof(Vec3), vertex_count, index_count);

	GpuBuffer *staging = device->alloc_stage(cube_mesh.get_vertex_buffer_size() + cube_mesh.get_index_buffer_size());

	cube_mesh.write_to_staging_buffer(staging, 0, vertices, indices);

	device->submit_graphics_immediate([&](CommandBuffer &cmd) {
		cube_mesh.batch_upload(cmd, staging, 0);
	});

	device->destroy_buffer(staging);
}

void DebugRenderer::render(float dt, RenderGraph &graph, RenderResourceHandle target_colour, RenderResourceHandle target_depth)
{
	depth_enabled_id = 0;
	depth_disabled_id = 0;

	for (int i = 0; i < DRAW_CALL_MAX_ENUM; i++) {
		auto filter_buckets = [&](Vector<DebugDrawCall> &calls) {
			for (int j = 0; j < calls.size();) {
				auto &call = calls[j];

				if (call.duration < 0.f) {
					calls.erase(calls.begin() + j);
				} else {
					call.duration -= dt;
					j++;
				}
			}
		};

		filter_buckets(depth_enabled_buckets[i]);
		filter_buckets(depth_disabled_buckets[i]);
	}

	struct Batch {
		DrawCallType type;
		u32 start;
		u32 count;
	};

	Vector<Batch> depth_batches;
	Vector<Batch> no_depth_batches;

	auto process_buckets = [&](Vector<DebugDrawCall> *buckets, Vector<Batch> &out, bool depth) {
		current_depth_enabled = depth;

		for (int type = 0; type < DRAW_CALL_MAX_ENUM; type++) {
			if (buckets[type].empty())
				continue;

			Batch batch = { (DrawCallType)type, depth ? depth_enabled_id : depth_disabled_id, (u32)buckets[type].size() };

			for (auto &call : buckets[type]) {
				current_colour = call.colour;
				current_thickness = call.line_width;
				current_alpha = (call.initial_duration <= CalcF::epsilon()) ? 1.f : CalcF::clamp(call.duration / call.initial_duration, 0.f, 1.f);
            
				switch (type) {
					case DRAW_CALL_LINE:      render_line      (call);  break;
					case DRAW_CALL_CROSS:     render_cross     (call);  break;
					case DRAW_CALL_SPHERE:    render_sphere    (call);  break;
					case DRAW_CALL_CIRCLE:    render_circle    (call);  break;
					case DRAW_CALL_TRIANGLE:  render_triangle  (call);  break;
					case DRAW_CALL_AABB:      render_aabb      (call);  break;
					case DRAW_CALL_OBB:       render_obb       (call);  break;
				}
			}

			out.push_back(batch);
		}
	};

	process_buckets(depth_enabled_buckets, depth_batches, true);
	process_buckets(depth_disabled_buckets, no_depth_batches, false);

	RenderStage &debug_rendering_stage = graph.push_stage("Debug Rendering", RenderStage::TYPE_GRAPHICS);
	debug_rendering_stage.write_colour(target_colour);
	debug_rendering_stage.write_depth(target_depth);

	debug_rendering_stage.set_record([=](const RenderContext &ctx, const RenderStageResources &resources) -> void {
		CommandBuffer &cmd = ctx.cmd;

		GraphicsPipelineDef pipeline_def(assets->get_asset<ast::ShaderAsset>(shader_asset)->shader);
		pipeline_def.topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
		pipeline_def.colour_attachment_formats = { VK_FORMAT_R32G32B32A32_SFLOAT };
		pipeline_def.cull_mode = VK_CULL_MODE_NONE;
		pipeline_def.blend_state.colour.src = VK_BLEND_FACTOR_SRC_ALPHA;
		pipeline_def.blend_state.colour.dst = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		pipeline_def.blend_state.colour.op = VK_BLEND_OP_ADD;
		pipeline_def.blend_state.alpha.src = VK_BLEND_FACTOR_ONE;
		pipeline_def.blend_state.alpha.dst = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		pipeline_def.blend_state.alpha.op = VK_BLEND_OP_ADD;
		pipeline_def.has_depth_attachment = true;
		pipeline_def.depth_stencil_state.depth_test_enabled = true;
		pipeline_def.depth_stencil_state.depth_write_enabled = false;
		pipeline_def.blend_state.enabled = true;

		PipelineState pipeline_st = ctx.cache.fetch_pipeline(pipeline_def);

		pipeline_def.depth_stencil_state.depth_test_enabled = false;
		PipelineState pipeline_st_no_depth = ctx.cache.fetch_pipeline(pipeline_def);

		struct {
			Mat4 view_proj;
			u64 calls_buffer;
			u64 vertex_buffer;
		} args;
	
		args.view_proj = ctx.camera.get_projection() * ctx.camera.get_view();

		auto draw_batches = [&](const Vector<Batch> &batches, u64 buffer_addr, PipelineState &st) {
			cmd.bind_pipeline(st.bind_point, st.pipeline);
			
			args.calls_buffer = buffer_addr;

			for (const auto &batch : batches) {
				auto render_batch = [&](Mesh &mesh) -> void {
					args.vertex_buffer = mesh.vertex_buffer->get_device_address();
					cmd.push_constants(st.layout, VK_SHADER_STAGE_ALL_GRAPHICS, sizeof(args), &args);
					cmd.bind_index_buffer(mesh.index_buffer, 0);
					cmd.draw_indexed(mesh.index_count, batch.count, 0, 0, batch.start);
				};

				switch (batch.type) {
					case DRAW_CALL_LINE:    render_batch(line_mesh);    break;
					case DRAW_CALL_CROSS:   render_batch(cross_mesh);   break;
					case DRAW_CALL_SPHERE:  render_batch(sphere_mesh);  break;
					case DRAW_CALL_CIRCLE:  render_batch(circle_mesh);  break;
					case DRAW_CALL_AABB:    render_batch(cube_mesh);    break;
					case DRAW_CALL_OBB:     render_batch(cube_mesh);    break;

					case DRAW_CALL_TRIANGLE: {
						render_batch(line_mesh);
						render_batch(line_mesh);
						render_batch(line_mesh);
					} break;
				}
			}
		};

		draw_batches(depth_batches, depth_enabled_call_buffer->get_device_address(), pipeline_st);
		draw_batches(no_depth_batches, depth_disabled_call_buffer->get_device_address(), pipeline_st_no_depth);
	});
}

void DebugRenderer::render_line(const DebugDrawCall &call)
{
	render_line_internal(call.line.from, call.line.to);
}

void DebugRenderer::render_cross(const DebugDrawCall &call)
{
	const Vec3 &point = call.cross.point;
	const float size = call.cross.size * 0.5f;

	Mat4 transform = Mat4::translate(point) * Mat4::scale(Vec3(size));

	push_instance_data(transform);
}

void DebugRenderer::render_sphere(const DebugDrawCall &call)
{
	const Vec3 &centre = call.sphere.centre;
	const float radius = call.sphere.radius;

	Mat4 transform = Mat4::translate(centre) * Mat4::scale(Vec3(radius));

	push_instance_data(transform);
}

void DebugRenderer::render_circle(const DebugDrawCall &call)
{
	const Vec3 &centre = call.circle.centre;
	const float radius = call.circle.radius;
	const Vec3 &normal = call.circle.normal;

	Mat4 transform = Mat4::translate(centre) * Mat4::rotate_around(0.f, normal) * Mat4::scale(Vec3(radius));

	push_instance_data(transform);
}

void DebugRenderer::render_triangle(const DebugDrawCall &call)
{
	const Vec3 &v0 = call.triangle.v0;
	const Vec3 &v1 = call.triangle.v1;
	const Vec3 &v2 = call.triangle.v2;

	render_line_internal(v0, v1);
	render_line_internal(v1, v2);
	render_line_internal(v2, v0);
}

void DebugRenderer::render_aabb(const DebugDrawCall &call)
{
	Vec3 centre = (call.aabb.max + call.aabb.min) * 0.5f;
	Vec3 size = (call.aabb.max - call.aabb.min);

	Mat4 transform = Mat4::translate(centre) * Mat4::scale(size);

	push_instance_data(transform);
}

void DebugRenderer::render_obb(const DebugDrawCall &call)
{
	Mat4 transform = call.obb.transform * Mat4::scale(call.obb.scale);

	push_instance_data(transform);
}

void DebugRenderer::render_line_internal(const Vec3 &from, const Vec3 &to)
{
	Vec3 direction = (to - from).normalized();
	float length = (to - from).length();

	Mat4 transform = Mat4::translate(from) * Mat4::rotate_around(0.f, direction) * Mat4::scale(Vec3(length));

	push_instance_data(transform);
}

void DebugRenderer::push_instance_data(const Mat4 &transform)
{
	float colour_alpha = (float)current_colour.a / 255.f;

	Vec4 colour = Vec4(
		colour_alpha * (float)current_colour.r / 255.f,
		colour_alpha * (float)current_colour.g / 255.f,
		colour_alpha * (float)current_colour.b / 255.f,
		colour_alpha * current_alpha
	);

	if (current_depth_enabled) {
		assert(depth_enabled_id < MAX_DEBUG_DRAWS);

		GPU_DebugObjectDraw *draws = (GPU_DebugObjectDraw *)depth_enabled_call_buffer->map();

		draws[depth_enabled_id].transform = transform;
		draws[depth_enabled_id].colour    = colour;
		draws[depth_enabled_id].thickness = current_thickness;

		depth_enabled_id++;
	} else {
		assert(depth_disabled_id < MAX_DEBUG_DRAWS);

		GPU_DebugObjectDraw *draws = (GPU_DebugObjectDraw *)depth_disabled_call_buffer->map();

		draws[depth_disabled_id].transform = transform;
		draws[depth_disabled_id].colour    = colour;
		draws[depth_disabled_id].thickness = current_thickness;

		depth_disabled_id++;
	}
}

void DebugRenderer::push_line(
	const Vec3 &from,
	const Vec3 &to,
	const Colour &colour,
	float line_width,
	float duration,
	bool depth_enabled
)
{
	DebugDrawCall call = {};
	call.colour = colour;
	call.line_width = line_width;
	call.duration = duration;
	call.initial_duration = duration;

	call.line.from = from;
	call.line.to = to;

	push_draw_call(call, DRAW_CALL_LINE, depth_enabled);
}

void DebugRenderer::push_cross(
	const Vec3 &point,
	float size,
	const Colour &colour,
	float duration,
	bool depth_enabled
)
{
	DebugDrawCall call = {};
	call.colour = colour;
	call.line_width = 1.f;
	call.duration = duration;
	call.initial_duration = duration;

	call.cross.point = point;
	call.cross.size = size;
	
	push_draw_call(call, DRAW_CALL_CROSS, depth_enabled);
}

void DebugRenderer::push_sphere(
	const Vec3 &centre,
	float radius,
	const Colour &colour,
	float duration,
	bool depth_enabled
)
{
	DebugDrawCall call = {};
	call.colour = colour;
	call.line_width = 1.f;
	call.duration = duration;
	call.initial_duration = duration;

	call.sphere.centre = centre;
	call.sphere.radius = radius;
	
	push_draw_call(call, DRAW_CALL_SPHERE, depth_enabled);
}

void DebugRenderer::push_circle(
	const Vec3 &centre,
	float radius,
	const Vec3 &plane_normal,
	const Colour &colour,
	float duration,
	bool depth_enabled
)
{
	DebugDrawCall call = {};
	call.colour = colour;
	call.line_width = 1.f;
	call.duration = duration;
	call.initial_duration = duration;

	call.circle.centre = centre;
	call.circle.radius = radius;
	call.circle.normal = plane_normal;
	
	push_draw_call(call, DRAW_CALL_CIRCLE, depth_enabled);
}

void DebugRenderer::push_triangle(
	const Vec3 &v0,
	const Vec3 &v1,
	const Vec3 &v2,
	const Colour &colour,
	float line_width,
	float duration,
	bool depth_enabled
)
{
	DebugDrawCall call = {};
	call.colour = colour;
	call.line_width = line_width;
	call.duration = duration;
	call.initial_duration = duration;

	call.triangle.v0 = v0;
	call.triangle.v1 = v1;
	call.triangle.v2 = v2;
	
	push_draw_call(call, DRAW_CALL_TRIANGLE, depth_enabled);
}

void DebugRenderer::push_aabb(
	const Vec3 &min,
	const Vec3 &max,
	const Colour &colour,
	float line_width,
	float duration,
	bool depth_enabled
)
{
	DebugDrawCall call = {};
	call.colour = colour;
	call.line_width = line_width;
	call.duration = duration;
	call.initial_duration = duration;

	call.aabb.min = min;
	call.aabb.max = max;
	
	push_draw_call(call, DRAW_CALL_AABB, depth_enabled);
}

void DebugRenderer::push_obb(
	const Mat4 &transform,
	const Vec3 &scale,
	const Colour &colour,
	float line_width,
	float duration,
	bool depth_enabled
)
{
	DebugDrawCall call = {};
	call.colour = colour;
	call.line_width = line_width;
	call.duration = duration;
	call.initial_duration = duration;

	call.obb.transform = transform;
	call.obb.scale = scale;
	
	push_draw_call(call, DRAW_CALL_OBB, depth_enabled);
}

void DebugRenderer::push_draw_call(
	const DebugDrawCall &call,
	DrawCallType type,
	bool depth_enabled
)
{
	if (depth_enabled)
		depth_enabled_buckets[type].push_back(call);
	else
		depth_disabled_buckets[type].push_back(call);
}
