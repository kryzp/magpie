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
	: draw_calls()
	, shader(nullptr)
	, current_layout()
	, current_view_proj()
	, current_colour()
	, current_line_width()
{
}

DebugRenderer::~DebugRenderer()
{
}

void DebugRenderer::init(ast::AssetManager &assets)
{
	this->shader = assets.get_asset<ast::ShaderAsset>(assets.from_file_path("assets://debug_rendering.msh"))->shader;
}

void DebugRenderer::destroy()
{
}

void DebugRenderer::clear()
{
	draw_calls.clear();
}

struct DebugLineDraw {
	Vec4 from;
	Vec4 to;
	Vec4 colour;
	float thickness;
};

void DebugRenderer::render(RenderGraph &graph, RenderResourceHandle target)
{
	RenderStage &debug_rendering_stage = graph.push_stage("Debug Rendering", RenderStage::TYPE_GRAPHICS);

	debug_rendering_stage.write_colour(target);

	debug_rendering_stage.set_record([&](const RenderContext &ctx, const RenderStageResources &resources) -> void {
		CommandBuffer &cmd = ctx.cmd;

		GraphicsPipelineDef pipeline_def(shader);
		pipeline_def.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
		pipeline_def.colour_attachment_formats = { VK_FORMAT_R32G32B32A32_SFLOAT };
		pipeline_def.depth_stencil_state.depth_test_enabled = false;
		pipeline_def.cull_mode = VK_CULL_MODE_NONE;

		PipelineState pipeline_st = ctx.cache.fetch_pipeline(pipeline_def);

		cmd.bind_pipeline(pipeline_st.bind_point, pipeline_st.pipeline);

		current_layout = pipeline_st.layout;
		current_view_proj = ctx.camera.get_projection() * ctx.camera.get_view();

		for (auto &call : draw_calls) {
			current_colour = call.colour;
			current_line_width = call.line_width;
			render_call(call, cmd);
		}
	});
}

void DebugRenderer::render_call(const DebugDrawCall &call, CommandBuffer &cmd)
{
	switch (call.type) {
		case DRAW_CALL_LINE:      render_line      (call, cmd);  break;
		case DRAW_CALL_CROSS:     render_cross     (call, cmd);  break;
		case DRAW_CALL_SPHERE:    render_sphere    (call, cmd);  break;
		case DRAW_CALL_CIRCLE:    render_circle    (call, cmd);  break;
		case DRAW_CALL_TRIANGLE:  render_triangle  (call, cmd);  break;
		case DRAW_CALL_AABB:      render_aabb      (call, cmd);  break;
		case DRAW_CALL_OBB:       render_obb       (call, cmd);  break;
	}
}

void DebugRenderer::render_line(const DebugDrawCall &call, CommandBuffer &cmd)
{
	render_line_internal(cmd,
		call.line.from,
		call.line.to
	);
}

void DebugRenderer::render_cross(const DebugDrawCall &call, CommandBuffer &cmd)
{
	// sqrt(d^2 + d^2) = 1/2
	// d = 1/(2 * sqrt(2)) = sqrt(2) / 4

	const Vec3 &point = call.cross.point;
	const float size = call.cross.size * CalcF::SQRT2 * 0.25f;

	render_line_internal(cmd,
		point - Vec3(size, size, size),
		point + Vec3(size, size, size)
	);

	render_line_internal(cmd,
		point - Vec3(-size, size, size),
		point + Vec3(-size, size, size)
	);
	
	render_line_internal(cmd,
		point - Vec3(size, -size, size),
		point + Vec3(size, -size, size)
	);
	
	render_line_internal(cmd,
		point - Vec3(size, size, -size),
		point + Vec3(size, size, -size)
	);
}

void DebugRenderer::render_sphere(const DebugDrawCall &call, CommandBuffer &cmd)
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

			render_line_internal(cmd, from, to_p);
			render_line_internal(cmd, from, to_t);
		}
	}
}

void DebugRenderer::render_circle(const DebugDrawCall &call, CommandBuffer &cmd)
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

		render_line_internal(cmd, from, to);
	}
}

void DebugRenderer::render_triangle(const DebugDrawCall &call, CommandBuffer &cmd)
{
	const Vec3 &v0 = call.triangle.v0;
	const Vec3 &v1 = call.triangle.v1;
	const Vec3 &v2 = call.triangle.v2;

	render_line_internal(cmd, v0, v1);
	render_line_internal(cmd, v1, v2);
	render_line_internal(cmd, v2, v0);
}

void DebugRenderer::render_aabb(const DebugDrawCall &call, CommandBuffer &cmd)
{
	const Vec3 &lo = call.aabb.min;
	const Vec3 &hi = call.aabb.max;

	// ---

	render_line_internal(cmd,
		Vec3(lo.x, lo.y, lo.z),
		Vec3(hi.x, lo.y, lo.z)
	);

	render_line_internal(cmd,
		Vec3(lo.x, lo.y, lo.z),
		Vec3(lo.x, hi.y, lo.z)
	);

	render_line_internal(cmd,
		Vec3(lo.x, lo.y, lo.z),
		Vec3(lo.x, lo.y, hi.z)
	);

	// ---

	render_line_internal(cmd,
		Vec3(hi.x, hi.y, hi.z),
		Vec3(lo.x, hi.y, hi.z)
	);

	render_line_internal(cmd,
		Vec3(hi.x, hi.y, hi.z),
		Vec3(hi.x, lo.y, hi.z)
	);

	render_line_internal(cmd,
		Vec3(hi.x, hi.y, hi.z),
		Vec3(hi.x, hi.y, lo.z)
	);

	// ---

	render_line_internal(cmd,
		Vec3(lo.x, hi.y, lo.z),
		Vec3(hi.x, hi.y, lo.z)
	);

	render_line_internal(cmd,
		Vec3(lo.x, hi.y, lo.z),
		Vec3(lo.x, hi.y, hi.z)
	);

	// ---

	render_line_internal(cmd,
		Vec3(hi.x, lo.y, lo.z),
		Vec3(hi.x, hi.y, lo.z)
	);

	render_line_internal(cmd,
		Vec3(hi.x, lo.y, lo.z),
		Vec3(hi.x, lo.y, hi.z)
	);

	// ---

	render_line_internal(cmd,
		Vec3(lo.x, lo.y, hi.z),
		Vec3(lo.x, hi.y, hi.z)
	);

	render_line_internal(cmd,
		Vec3(lo.x, lo.y, hi.z),
		Vec3(hi.x, lo.y, hi.z)
	);
}

void DebugRenderer::render_obb(const DebugDrawCall &call, CommandBuffer &cmd)
{
	// TODO
}

void DebugRenderer::render_line_internal(
	CommandBuffer &cmd,
	const Vec3 &from,
	const Vec3 &to
)
{
	struct {
		Mat4 view_proj;
		Vec4 from_position;
		Vec4 to_position;
		Vec4 colour;
		Vec2 resolution;
		float thickness;
	} args;
	
	args.view_proj = current_view_proj;

	args.from_position.x = from.x;
	args.from_position.y = from.y;
	args.from_position.z = from.z;
	args.from_position.w = 1.f;

	args.to_position.x = to.x;
	args.to_position.y = to.y;
	args.to_position.z = to.z;
	args.to_position.w = 1.f;

	args.colour.x = (float)current_colour.r / 255.f;
	args.colour.y = (float)current_colour.g / 255.f;
	args.colour.z = (float)current_colour.b / 255.f;
	args.colour.w = 1.f;

	args.resolution.x = 1280.f;
	args.resolution.y = 720.f;

	args.thickness = current_line_width;

	cmd.push_constants(current_layout, VK_SHADER_STAGE_ALL_GRAPHICS, sizeof(args), &args);
	cmd.draw_vertices_n(4);
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
	call.depth_enabled = depth_enabled;

	call.obb.transform = transform;
	call.obb.scale = scale;

	draw_calls.push_back(call);
}
