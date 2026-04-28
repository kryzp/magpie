#ifndef RENDER_PASS_DEBUG_H
#define RENDER_PASS_DEBUG_H

#define R_DEBUG_MAX_DRAWS    65536
#define R_DEBUG_MAX_BATCHES  64

typedef enum R_DebugDrawType
{
	R_DebugDrawType_Line,
	R_DebugDrawType_Cross,
	R_DebugDrawType_Sphere,
	R_DebugDrawType_Circle,
	R_DebugDrawType_Triangle,
	R_DebugDrawType_AABB,
	R_DebugDrawType_OBB,
	R_DebugDrawType_COUNT
}
R_DebugDrawType;

typedef struct R_DebugDrawCall R_DebugDrawCall;
struct R_DebugDrawCall
{
	v4 colour;
	f32 line_width;
	f32 duration;
	f32 initial_duration;

	union
	{
		struct
		{
			v3 from;
			v3 to;
		}
		line;
		
		struct
		{
			v3 point;
			f32 size;
		}
		cross;
		
		struct
		{
			v3 centre;
			f32 radius;
		}
		sphere;
		
		struct
		{
			v3 centre;
			f32 radius;
			v3 normal;
		}
		circle;
		
		struct
		{
			v3 v0;
			v3 v1;
			v3 v2;
		}
		triangle;
		
		struct
		{
			v3 min;
			v3 max;
		}
		aabb;
		
		struct
		{
			m4 transform;
			v3 scale;
		}
		obb;
	};
};

typedef struct R_DebugDrawNode R_DebugDrawNode;
struct R_DebugDrawNode
{
	R_DebugDrawNode *next;
	R_DebugDrawType type;
	R_DebugDrawCall call;
};

typedef struct R_DebugRenderer R_DebugRenderer;
struct R_DebugRenderer
{
	Arena *arena;
	GFX_Device *device;
	AST_Assets *assets;

	R_DebugDrawNode *free_list;

	R_DebugDrawNode *depth_enabled  [R_DebugDrawType_COUNT];
	R_DebugDrawNode *depth_disabled [R_DebugDrawType_COUNT];

	GFX_BufferKey depth_enabled_buffer;
	GFX_BufferKey depth_disabled_buffer;

	R_Mesh line_mesh;
	R_Mesh cross_mesh;
	R_Mesh sphere_mesh;
	R_Mesh circle_mesh;
	R_Mesh cube_mesh;

	AST_Handle shader_handle;
};


/* ==================================================
   CORE
   ================================================== */

internal void R_DebugRendererInit    (R_DebugRenderer *dr, Arena *arena, GFX_Device *device, AST_Assets *assets);
internal void R_DebugRendererDestroy (R_DebugRenderer *dr);


/* ==================================================
   RENDERING
   ================================================== */

internal void R_DebugRendererRender (R_DebugRenderer *dr,
									 f32 dt,
									 R_Graph *graph,
									 Arena *pass_arena,
									 R_GraphTexHandle target_colour,
									 R_GraphTexHandle target_depth);


/* ==================================================
   INTERFACE
   ================================================== */

internal void R_DebugPushLine    (R_DebugRenderer *dr,
								  v3 from, v3 to,
								  v4 colour,
								  f32 line_width,
								  f32 duration,
								  b32 depth_enabled);

internal void R_DebugPushCross    (R_DebugRenderer *dr,
								   v3 point, f32 size,
								   v4 colour,
								   f32 duration,
								   b32 depth_enabled);

internal void R_DebugPushSphere   (R_DebugRenderer *dr,
								   v3 centre, f32 radius,
								   v4 colour,
								   f32 duration,
								   b32 depth_enabled);

internal void R_DebugPushCircle   (R_DebugRenderer *dr,
								   v3 centre, f32 radius, v3 plane_normal,
								   v4 colour,
								   f32 duration,
								   b32 depth_enabled);

internal void R_DebugPushTriangle (R_DebugRenderer *dr,
								   v3 a, v3 b, v3 c,
								   v4 colour,
								   f32 line_width,
								   f32 duration,
								   b32 depth_enabled);

internal void R_DebugPushAABB     (R_DebugRenderer *dr,
								   v3 min, v3 max,
								   v4 colour,
								   f32 line_width,
								   f32 duration,
								   b32 depth_enabled);

internal void R_DebugPushOBB      (R_DebugRenderer *dr,
								   m4 transform, v3 scale,
								   v4 colour,
								   f32 line_width,
								   f32 duration,
								   b32 depth_enabled);

#endif // RENDER_PASS_DEBUG_H
