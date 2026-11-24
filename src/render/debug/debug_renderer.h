#ifndef RENDER_DEBUG_RENDERER_H
#define RENDER_DEBUG_RENDERER_H

#include "core/types.h"
#include "core/maths.h"
#include "os/spin_lock.h"
#include "render/mesh.h"
#include "render/frame.h"
#include "render/graph/graph_types.h"
#include "container/vector.h"

#define RENDER_DEBUG_MAX_DRAWS 65536

namespace rnd {

class Graph;

enum DebugDrawType : unsigned {
	DebugDrawType_Line,
	DebugDrawType_Cross,
	DebugDrawType_Sphere,
	DebugDrawType_Circle,
	DebugDrawType_Triangle,
	DebugDrawType_AABB,
	DebugDrawType_OBB,
	DebugDrawType_MaxEnum
};

struct DebugDrawCall {
	Vec4 colour;
	float line_width = 1.f;
	float duration = 0.f;
	float initial_duration = 0.f;

	Vec3 a;                 // line.from, cross.point, sphere/circle.centre, triangle.v0, aabb.min
	Vec3 b;                 // line.to, circle.normal, triangle.v1, aabb.max, obb.scale
	Vec3 c;                 // triangle.v2
	float size = 0.f;       // cross.size, sphere/circle.radius
	Mat4 transform;         // obb.transform
};

class DebugRenderer {
public:
	DebugRenderer();
	~DebugRenderer();

	DebugRenderer(const DebugRenderer &) = delete;
	DebugRenderer &operator=(const DebugRenderer &) = delete;

	static DebugRenderer *Get();
	static void Select(DebugRenderer *renderer);

	/* ==================================================
	   RENDERING
	   ================================================== */

	void CollectInto(FramePacket *packet, float dt);

	void Render(
		Graph *graph,
		const FrameParams *frame_params,
		GraphTexHandle target_colour,
		GraphTexHandle target_depth
	);

	const Mesh *MeshForType(DebugDrawType type) const;

	/* ==================================================
	   INTERFACE
	   ================================================== */

	void PushLine(
		const Vec3 &from, const Vec3 &to,
		const Vec4 &colour,
		float line_width,
		float duration,
		bool depth_enabled
	);

	void PushCross(
		const Vec3 &point, float size,
		const Vec4 &colour,
		float duration,
		bool depth_enabled
	);

	void PushSphere(
		const Vec3 &centre, float radius,
		const Vec4 &colour,
		float duration,
		bool depth_enabled
	);

	void PushCircle(
		const Vec3 &centre, float radius, const Vec3 &plane_normal,
		const Vec4 &colour,
		float duration,
		bool depth_enabled
	);

	void PushTriangle(
		const Vec3 &a, const Vec3 &b, const Vec3 &c,
		const Vec4 &colour,
		float line_width,
		float duration,
		bool depth_enabled
	);

	void PushAABB(
		const Vec3 &min, const Vec3 &max,
		const Vec4 &colour,
		float line_width,
		float duration,
		bool depth_enabled
	);

	void PushOBB(
		const Mat4 &transform, const Vec3 &scale,
		const Vec4 &colour,
		float line_width,
		float duration,
		bool depth_enabled
	);

private:
	typedef Vector<DebugDrawCall> Bucket;

	void PushDrawCall(DebugDrawType type, const DebugDrawCall &call, bool depth_enabled);

	static void FilterBuckets(Bucket *buckets, float dt);
	static unsigned CountInstances(const Bucket *buckets);
	static void BuildInstances(const Bucket *buckets, Vector<GPU_DebugObjectDraw> *draws);
	static unsigned BuildBatches(const Bucket *buckets, DebugBatch *out_batches, unsigned *running_id);

	SpinLock spinlock;

	// sort draw calls into buckets
	Bucket depth_enabled[DebugDrawType_MaxEnum];
	Bucket depth_disabled[DebugDrawType_MaxEnum];

	Mesh line_mesh;
	Mesh cross_mesh;
	Mesh sphere_mesh;
	Mesh circle_mesh;
	Mesh cube_mesh;
};
}

#endif // RENDER_DEBUG_RENDERER_H
