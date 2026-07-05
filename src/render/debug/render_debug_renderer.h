#ifndef RENDER_DEBUG_RENDERER_H
#define RENDER_DEBUG_RENDERER_H

#define R_DEBUG_MAX_DRAWS    65536
#define R_DEBUG_MAX_BATCHES  64

typedef struct R_GPU_DebugObjectDraw R_GPU_DebugObjectDraw;
struct R_GPU_DebugObjectDraw
{
	m4 transform;
	v4 colour;
	f32 thickness;
};

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
	G_Device *device;
	A_Assets *assets;

	R_DebugDrawNode *free_list;

	// sort draw calls into buckets
	R_DebugDrawNode *depth_enabled  [R_DebugDrawType_COUNT];
	R_DebugDrawNode *depth_disabled [R_DebugDrawType_COUNT];

	G_BufferKey depth_enabled_buffer;
	G_BufferKey depth_disabled_buffer;

	R_Mesh line_mesh;
	R_Mesh cross_mesh;
	R_Mesh sphere_mesh;
	R_Mesh circle_mesh;
	R_Mesh cube_mesh;

	A_Handle shader_handle;
};


/* ==================================================
   CORE
   ================================================== */

static void R_DebugRendererInitAndSelect (R_DebugRenderer *dr, Arena *arena, G_Device *device, A_Assets *assets);
static void R_DebugRendererDestroy       (R_DebugRenderer *dr);

static void R_DebugRendererSelect        (R_DebugRenderer *dr);


/* ==================================================
   RENDERING
   ================================================== */

static void R_DebugRendererRender (R_DebugRenderer *dr,
									 R_Graph *graph,
									 const R_FrameParams *frame_params,
									 R_GraphTexHandle target_colour,
									 R_GraphTexHandle target_depth);


/* ==================================================
   INTERFACE
   ================================================== */

static void R_DebugPushLine     (v3 from, v3 to,
								   v4 colour,
								   f32 line_width,
								   f32 duration,
								   b32 depth_enabled);

static void R_DebugPushCross    (v3 point, f32 size,
								   v4 colour,
								   f32 duration,
								   b32 depth_enabled);

static void R_DebugPushSphere   (v3 centre, f32 radius,
								   v4 colour,
								   f32 duration,
								   b32 depth_enabled);

static void R_DebugPushCircle   (v3 centre, f32 radius, v3 plane_normal,
								   v4 colour,
								   f32 duration,
								   b32 depth_enabled);

static void R_DebugPushTriangle (v3 a, v3 b, v3 c,
								   v4 colour,
								   f32 line_width,
								   f32 duration,
								   b32 depth_enabled);

static void R_DebugPushAABB     (v3 min, v3 max,
								   v4 colour,
								   f32 line_width,
								   f32 duration,
								   b32 depth_enabled);

static void R_DebugPushOBB      (m4 transform, v3 scale,
								   v4 colour,
								   f32 line_width,
								   f32 duration,
								   b32 depth_enabled);

#endif // RENDER_DEBUG_RENDERER_H
