#include "debug_renderer.h"

#include "assets/shader_serializer.h"
#include "math/calc.h"

#include "../camera.h"

using namespace gfx;

DebugRenderer *DebugRenderer::get_singleton()
{
	static DebugRenderer instance;
	return &instance;
}

DebugRenderer::DebugRenderer()
	: device(nullptr)
	, draw_calls()
	, shader(nullptr)
	, depth_enabled_call_buffer(nullptr)
	, depth_enabled_id(0)
	, no_depth_call_buffer(nullptr)
	, no_depth_id(0)
	, current_colour()
	, current_thickness()
	, current_alpha(0.f)
{
}

DebugRenderer::~DebugRenderer()
{
}

struct GPU_DebugLineDraw {
	Vec4 from;
	Vec4 to;
	Vec4 colour;
	float thickness;
};

void DebugRenderer::init(Device *device, ast::AssetManager &assets)
{
	this->device = device;

	this->shader = assets.get_asset<ast::ShaderAsset>(assets.from_file_path("assets://debug_rendering.msh"))->shader;
	
	this->depth_enabled_call_buffer = device->alloc_buffer(
		VK_BUFFER_USAGE_2_STORAGE_BUFFER_BIT |
		VK_BUFFER_USAGE_2_TRANSFER_DST_BIT,
		VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
		sizeof(GPU_DebugLineDraw) * MAX_DEBUG_DRAWS
	);

	this->no_depth_call_buffer = device->alloc_buffer(
		VK_BUFFER_USAGE_2_STORAGE_BUFFER_BIT |
		VK_BUFFER_USAGE_2_TRANSFER_DST_BIT,
		VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
		sizeof(GPU_DebugLineDraw) * MAX_DEBUG_DRAWS
	);
}

void DebugRenderer::destroy()
{
	device->destroy_buffer(depth_enabled_call_buffer);
	device->destroy_buffer(no_depth_call_buffer);
}

void DebugRenderer::render(float dt, RenderGraph &graph, RenderResourceHandle target_colour, RenderResourceHandle target_depth)
{
	depth_enabled_id = 0;
	no_depth_id = 0;

	for (int i = 0; i < draw_calls.size();) {
		auto &call = draw_calls[i];

		if (call.duration < 0.f) {
			draw_calls.erase(draw_calls.begin() + i);
		} else {
			current_colour = call.colour;
			current_thickness = call.line_width;
			current_depth_enabled = call.depth_enabled;

			if (call.initial_duration <= CalcF::epsilon())
				current_alpha = 1.f;
			else
				current_alpha = CalcF::clamp(call.duration / call.initial_duration, 0.f, 1.f);

			render_call(call);

			call.duration -= dt;

			i++;
		}
	}

	RenderStage &debug_rendering_stage = graph.push_stage("Debug Rendering", RenderStage::TYPE_GRAPHICS);
	debug_rendering_stage.write_colour(target_colour);
	debug_rendering_stage.write_depth(target_depth);

	debug_rendering_stage.set_record([&](const RenderContext &ctx, const RenderStageResources &resources) -> void {
		CommandBuffer &cmd = ctx.cmd;

		GraphicsPipelineDef pipeline_def(shader);
		pipeline_def.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
		pipeline_def.colour_attachment_formats = { VK_FORMAT_R32G32B32A32_SFLOAT };
		pipeline_def.cull_mode = VK_CULL_MODE_NONE;
		pipeline_def.has_depth_attachment = true;
		pipeline_def.depth_stencil_state.depth_test_enabled = true;
		pipeline_def.depth_stencil_state.depth_write_enabled = false;
		pipeline_def.blend_state.enabled = true;
		pipeline_def.blend_state.colour.src = VK_BLEND_FACTOR_SRC_ALPHA;
		pipeline_def.blend_state.colour.dst = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		pipeline_def.blend_state.colour.op = VK_BLEND_OP_ADD;
		pipeline_def.blend_state.alpha.src = VK_BLEND_FACTOR_ONE;
		pipeline_def.blend_state.alpha.dst = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		pipeline_def.blend_state.alpha.op = VK_BLEND_OP_ADD;

		PipelineState pipeline_st = ctx.cache.fetch_pipeline(pipeline_def);

		pipeline_def.depth_stencil_state.depth_test_enabled = false;
		PipelineState pipeline_st_no_depth = ctx.cache.fetch_pipeline(pipeline_def);

		struct {
			Mat4 view_proj;
			u64 calls_buffer;
			Vec2 resolution;
		} args;
	
		args.view_proj = ctx.camera.get_projection() * ctx.camera.get_view();
		args.resolution.x = 1280.f;
		args.resolution.y = 720.f;

		// ---

		args.calls_buffer = depth_enabled_call_buffer->get_device_address();

		cmd.bind_pipeline(pipeline_st.bind_point, pipeline_st.pipeline);
		cmd.push_constants(pipeline_st.layout, VK_SHADER_STAGE_ALL_GRAPHICS, sizeof(args), &args);

		cmd.draw(4, depth_enabled_id > MAX_DEBUG_DRAWS ? MAX_DEBUG_DRAWS : depth_enabled_id, 0, 0);

		// ---

		args.calls_buffer = no_depth_call_buffer->get_device_address();

		cmd.bind_pipeline(pipeline_st_no_depth.bind_point, pipeline_st_no_depth.pipeline);
		cmd.push_constants(pipeline_st_no_depth.layout, VK_SHADER_STAGE_ALL_GRAPHICS, sizeof(args), &args);

		cmd.draw(4, no_depth_id > MAX_DEBUG_DRAWS ? MAX_DEBUG_DRAWS : no_depth_id, 0, 0);
	});
}

void DebugRenderer::render_call(const DebugDrawCall &call)
{
	switch (call.type) {
		case DRAW_CALL_LINE:      render_line      (call);  break;
		case DRAW_CALL_CROSS:     render_cross     (call);  break;
		case DRAW_CALL_SPHERE:    render_sphere    (call);  break;
		case DRAW_CALL_CIRCLE:    render_circle    (call);  break;
		case DRAW_CALL_TRIANGLE:  render_triangle  (call);  break;
		case DRAW_CALL_AABB:      render_aabb      (call);  break;
		case DRAW_CALL_OBB:       render_obb       (call);  break;
	}
}

void DebugRenderer::render_line(const DebugDrawCall &call)
{
	render_line_internal(
		call.line.from,
		call.line.to
	);
}

void DebugRenderer::render_cross(const DebugDrawCall &call)
{
	// sqrt(d^2 + d^2) = 1/2
	// d = 1/(2 * sqrt(2)) = sqrt(2) / 4

	const Vec3 &point = call.cross.point;
	const float size = call.cross.size * CalcF::SQRT2 * 0.25f;

	render_line_internal(
		point - Vec3(size, size, size),
		point + Vec3(size, size, size)
	);

	render_line_internal(
		point - Vec3(-size, size, size),
		point + Vec3(-size, size, size)
	);
	
	render_line_internal(
		point - Vec3(size, -size, size),
		point + Vec3(size, -size, size)
	);
	
	render_line_internal(
		point - Vec3(size, size, -size),
		point + Vec3(size, size, -size)
	);
}

void DebugRenderer::render_sphere(const DebugDrawCall &call)
{
	const Vec3 &centre = call.sphere.centre;
	const float radius = call.sphere.radius;

	const int resolution = 12;

	for (int i = 0; i < resolution; i++) {
		for (int j = 0; j < resolution; j++) {
			float i_p  = (i + 0) / (float)resolution;
			float i_pn = (i + 1) / (float)resolution;

			float j_p  = (j + 0) / (float)resolution;
			float j_pn = (j + 1) / (float)resolution;

			float theta      = CalcF::PI*i_p  - CalcF::PI*0.5f;
			float theta_next = CalcF::PI*i_pn - CalcF::PI*0.5f;

			float phi      = CalcF::TAU*j_p;
			float phi_next = CalcF::TAU*j_pn;

			Vec3 from = centre + Vec3::spherical_to_cartesian(radius, phi, theta);
			Vec3 to_p = centre + Vec3::spherical_to_cartesian(radius, phi_next, theta);
			Vec3 to_t = centre + Vec3::spherical_to_cartesian(radius, phi, theta_next);

			render_line_internal(from, to_p);
			render_line_internal(from, to_t);
		}
	}
}

void DebugRenderer::render_circle(const DebugDrawCall &call)
{
	const Vec3 &centre = call.circle.centre;
	const float radius = call.circle.radius;
	const Vec3 &normal = call.circle.normal;

	const int resolution = 12;

	for (int i = 0; i < resolution; i++) {
		float i_p  = (i + 0) / (float)resolution;
		float i_pn = (i + 1) / (float)resolution;

		float phi      = CalcF::TAU*i_p;
		float phi_next = CalcF::TAU*i_pn;

		Vec3 base = Vec3::spherical_to_cartesian(radius, phi, 0.f);
		Vec3 next = Vec3::spherical_to_cartesian(radius, phi_next, 0.f);

		Vec3 from = centre + Vec3::cross(normal, Vec3::cross(base, normal));
		Vec3 to   = centre + Vec3::cross(normal, Vec3::cross(next, normal));

		render_line_internal(from, to);
	}
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
	const Vec3 &lo = call.aabb.min;
	const Vec3 &hi = call.aabb.max;

	// ---

	render_line_internal(
		Vec3(lo.x, lo.y, lo.z),
		Vec3(hi.x, lo.y, lo.z)
	);

	render_line_internal(
		Vec3(lo.x, lo.y, lo.z),
		Vec3(lo.x, hi.y, lo.z)
	);

	render_line_internal(
		Vec3(lo.x, lo.y, lo.z),
		Vec3(lo.x, lo.y, hi.z)
	);

	// ---

	render_line_internal(
		Vec3(hi.x, hi.y, hi.z),
		Vec3(lo.x, hi.y, hi.z)
	);

	render_line_internal(
		Vec3(hi.x, hi.y, hi.z),
		Vec3(hi.x, lo.y, hi.z)
	);

	render_line_internal(
		Vec3(hi.x, hi.y, hi.z),
		Vec3(hi.x, hi.y, lo.z)
	);

	// ---

	render_line_internal(
		Vec3(lo.x, hi.y, lo.z),
		Vec3(hi.x, hi.y, lo.z)
	);

	render_line_internal(
		Vec3(lo.x, hi.y, lo.z),
		Vec3(lo.x, hi.y, hi.z)
	);

	// ---

	render_line_internal(
		Vec3(hi.x, lo.y, lo.z),
		Vec3(hi.x, hi.y, lo.z)
	);

	render_line_internal(
		Vec3(hi.x, lo.y, lo.z),
		Vec3(hi.x, lo.y, hi.z)
	);

	// ---

	render_line_internal(
		Vec3(lo.x, lo.y, hi.z),
		Vec3(lo.x, hi.y, hi.z)
	);

	render_line_internal(
		Vec3(lo.x, lo.y, hi.z),
		Vec3(hi.x, lo.y, hi.z)
	);
}

void DebugRenderer::render_obb(const DebugDrawCall &call)
{
	// TODO
}

void DebugRenderer::render_line_internal(const Vec3 &from, const Vec3 &to)
{
	if (current_depth_enabled) {
		assert(depth_enabled_id < MAX_DEBUG_DRAWS);

		GPU_DebugLineDraw *draws = (GPU_DebugLineDraw *)depth_enabled_call_buffer->map();

		Vec4 colour = Vec4(
			(float)current_colour.r / 255.f,
			(float)current_colour.g / 255.f,
			(float)current_colour.b / 255.f,
			current_alpha
		);

		draws[depth_enabled_id].from      = Vec4(from.x, from.y, from.z, 1.f);
		draws[depth_enabled_id].to        = Vec4(to.x,   to.y,   to.z,   1.f);
		draws[depth_enabled_id].colour    = colour;
		draws[depth_enabled_id].thickness = current_thickness;

		depth_enabled_id++;
	} else {
		assert(no_depth_id < MAX_DEBUG_DRAWS);

		GPU_DebugLineDraw *draws = (GPU_DebugLineDraw *)no_depth_call_buffer->map();

		Vec4 colour = Vec4(
			(float)current_colour.r / 255.f,
			(float)current_colour.g / 255.f,
			(float)current_colour.b / 255.f,
			current_alpha
		);

		draws[no_depth_id].from      = Vec4(from.x, from.y, from.z, 1.f);
		draws[no_depth_id].to        = Vec4(to.x,   to.y,   to.z,   1.f);
		draws[no_depth_id].colour    = colour;
		draws[no_depth_id].thickness = current_thickness;

		no_depth_id++;
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
	call.type = DRAW_CALL_LINE;
	call.colour = colour;
	call.line_width = line_width;
	call.duration = duration;
	call.initial_duration = duration;
	call.depth_enabled = depth_enabled;

	call.line.from = from;
	call.line.to = to;

	draw_calls.push_back(call);
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
	call.type = DRAW_CALL_CROSS;
	call.colour = colour;
	call.line_width = 1.f;
	call.duration = duration;
	call.initial_duration = duration;
	call.depth_enabled = depth_enabled;

	call.cross.point = point;
	call.cross.size = size;

	draw_calls.push_back(call);
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
	call.type = DRAW_CALL_SPHERE;
	call.colour = colour;
	call.line_width = 1.f;
	call.duration = duration;
	call.initial_duration = duration;
	call.depth_enabled = depth_enabled;

	call.sphere.centre = centre;
	call.sphere.radius = radius;

	draw_calls.push_back(call);
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
	call.type = DRAW_CALL_CIRCLE;
	call.colour = colour;
	call.line_width = 1.f;
	call.duration = duration;
	call.initial_duration = duration;
	call.depth_enabled = depth_enabled;

	call.circle.centre = centre;
	call.circle.radius = radius;
	call.circle.normal = plane_normal;

	draw_calls.push_back(call);
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
	call.type = DRAW_CALL_TRIANGLE;
	call.colour = colour;
	call.line_width = line_width;
	call.duration = duration;
	call.initial_duration = duration;
	call.depth_enabled = depth_enabled;

	call.triangle.v0 = v0;
	call.triangle.v1 = v1;
	call.triangle.v2 = v2;

	draw_calls.push_back(call);
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
	call.type = DRAW_CALL_AABB;
	call.colour = colour;
	call.line_width = line_width;
	call.duration = duration;
	call.initial_duration = duration;
	call.depth_enabled = depth_enabled;

	call.aabb.min = min;
	call.aabb.max = max;

	draw_calls.push_back(call);
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
	call.type = DRAW_CALL_OBB;
	call.colour = colour;
	call.line_width = line_width;
	call.duration = duration;
	call.initial_duration = duration;
	call.depth_enabled = depth_enabled;

	call.obb.transform = transform;
	call.obb.scale = scale;

	draw_calls.push_back(call);
}
