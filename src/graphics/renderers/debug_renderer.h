#pragma once

/*
 * Inspired by Game Engine Architecture (3rd Edition)'s debug rendering section.
 */

#include "container/vector.h"

#include "math/mat4.h"
#include "math/vec3.h"
#include "math/colour.h"

#include "assets/assets.h"

#include "../render_graph.h"

namespace gfx
{
	class DebugRenderer {
		constexpr static u32 MAX_DEBUG_DRAWS = 65536;

	public:
		static DebugRenderer *get_singleton();

		DebugRenderer();
		~DebugRenderer();

		void init(Device *device, ast::AssetManager &assets);
		void destroy();

		void render(float dt, RenderGraph &graph, RenderResourceHandle target_colour, RenderResourceHandle target_depth);

		void push_line(
			const Vec3 &from,
			const Vec3 &to,
			const Colour &colour,
			float line_width = 1.f,
			float duration = 0.f,
			bool depth_enabled = true
		);

		void push_cross(
			const Vec3 &point,
			float size,
			const Colour &colour,
			float duration = 0.f,
			bool depth_enabled = true
		);

		void push_sphere(
			const Vec3 &centre,
			float radius,
			const Colour &colour,
			float duration = 0.f,
			bool depth_enabled = true
		);

		void push_circle(
			const Vec3 &centre,
			float radius,
			const Vec3 &plane_normal,
			const Colour &colour,
			float duration = 0.f,
			bool depth_enabled = true
		);

		void push_triangle(
			const Vec3 &v0,
			const Vec3 &v1,
			const Vec3 &v2,
			const Colour &colour,
			float line_width = 1.f,
			float duration = 0.f,
			bool depth_enabled = true
		);

		void push_aabb(
			const Vec3 &min,
			const Vec3 &max,
			const Colour &colour,
			float line_width = 1.f,
			float duration = 0.f,
			bool depth_enabled = true
		);

		void push_obb(
			const Mat4 &transform,
			const Vec3 &scale,
			const Colour &colour,
			float line_width = 1.f,
			float duration = 0.f,
			bool depth_enabled = true
		);

	private:
		Device *device;

		enum DrawCallType {
			DRAW_CALL_LINE,
			DRAW_CALL_CROSS,
			DRAW_CALL_SPHERE,
			DRAW_CALL_CIRCLE,
			DRAW_CALL_TRIANGLE,
			DRAW_CALL_AABB,
			DRAW_CALL_OBB,
			DRAW_CALL_MAX_ENUM
		};

		struct DebugDrawCall {
			DrawCallType type;
			Colour colour;
			float line_width;
			float duration;
			float initial_duration;
			bool depth_enabled;

			union {
				struct {
					Vec3 from;
					Vec3 to;
				} line;

				struct {
					Vec3 point;
					float size;
				} cross;

				struct {
					Vec3 centre;
					float radius;
				} sphere;

				struct {
					Vec3 centre;
					float radius;
					Vec3 normal;
				} circle;

				struct {
					Vec3 v0;
					Vec3 v1;
					Vec3 v2;
				} triangle;

				struct {
					Vec3 min;
					Vec3 max;
				} aabb;

				struct {
					Mat4 transform;
					Vec3 scale;
				} obb;
			};
		};

		void render_call(const DebugDrawCall &call);

		void render_line(const DebugDrawCall &call);
		void render_cross(const DebugDrawCall &call);
		void render_sphere(const DebugDrawCall &cal);
		void render_circle(const DebugDrawCall &call);
		void render_triangle(const DebugDrawCall &call);
		void render_aabb(const DebugDrawCall &call);
		void render_obb(const DebugDrawCall &call);

		void render_line_internal(const Vec3 &from, const Vec3 &to);

		Vector<DebugDrawCall> draw_calls;

		const ShaderProgram *shader;

		GpuBuffer *depth_enabled_call_buffer;
		u32 depth_enabled_id;

		GpuBuffer *no_depth_call_buffer;
		u32 no_depth_id;

		Colour current_colour;
		float current_thickness;
		bool current_depth_enabled;
		float current_alpha;
	};
}
