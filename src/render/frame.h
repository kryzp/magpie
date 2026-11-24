#ifndef RENDER_FRAME_H
#define RENDER_FRAME_H

#include "core/types.h"
#include "core/maths.h"
#include "graphics/mesh.h"
#include "graphics/resource_key.h"
#include "graphics/ring_buffer.h"
#include "graphics/cmd_buffer.h"
#include "render/camera.h"
#include "render/gpu_types.h"
#include "render/light.h"
#include "render/mesh.h"
#include "render/scene.h"

#include "container/vector.h"

/*
 * FRAME PACKET - built by the game, sent over to rendering system
 * FRAME PARAMS - built from the packet, contains all the GPU-ready data
 */

#define RENDER_DEBUG_MAX_BATCHES                      64

#define RENDER_FRAME_SLOT_COUNT                       2
#define RENDER_FRAME_PARAMS_MAX_SHADOW_CASTERS        8

// note: don't make this too high,
//       remember BAR=256mb and we don't wanna
//       exceed that otherwise we start
//       falling back to PCIe
#define RENDER_FRAME_UPLOAD_BUDGET                    Megabytes(8)

namespace rnd {

class System;
class ShadowRenderer;

enum ObjectFlags : unsigned {
	ObjectFlag_None = 0,
	ObjectFlag_Translucent = 1 << 0,
};

struct PacketObject {
	Mat4 transform;
	Mat4 normal_matrix;
	Vec4 local_sphere_bounds;

	u32 page_index = 0;
	u32 mesh_index = 0;
	u32 material_index = 0;

	// copied by value during collect so animation system can rewrite pallette
	u32 skinning_first_matrix = 0;
	u32 skinning_joint_count = 0;

	u32 flags = ObjectFlag_None;
};

struct PacketGeometryPage {
	u64 vertex_buffer_address = 0;
	gfx::ResourceKey index_buffer;
	gfx::IndexType index_type = gfx::IndexType_U32;
};

enum DebugDepthMode {
	DebugDepthMode_Tested,  // depth test
	DebugDepthMode_Overlay, // no depth test
	DebugDepthMode_MaxEnum
};

struct DebugBatch {
	u32 type = 0; // DebugDrawType
	u32 start = 0;
	u32 count = 0;
};

struct DebugPacket {
	Vector<GPU_DebugObjectDraw> instances[DebugDepthMode_MaxEnum];
	unsigned batch_count[DebugDepthMode_MaxEnum];
	DebugBatch batches[DebugDepthMode_MaxEnum][RENDER_DEBUG_MAX_BATCHES];
};

// built on the game side each frame and handed to the render job, which
// only reads it.
// -- vectors keep their capacity between frames.
class FramePacket {
public:
	FramePacket();

	void Begin(u64 frame_number, float dt, float elapsed, unsigned window_width, unsigned window_height);
	void CollectScene(const Scene &scene, const Camera &camera, float interp_t);

	u64 frame_number;
	float dt;
	float elapsed;

	unsigned window_width;
	unsigned window_height;

	Camera camera;

	Vector<PacketObject> objects;
	Vector<Mat4> skinning_matrices;
	Vector<Light> lights;
	Vector<GPU_RenderMesh> meshes;
	Vector<GPU_Material> materials;

	unsigned page_count;
	PacketGeometryPage pages[RENDER_SCENE_MAX_GEOMETRY_PAGES];

	DebugPacket debug;
};

struct ShadowCaster {
	Vec3 position;
	float far_plane = 0.f;
	float near_plane = 0.f;
	float radius = 0.f;

	// hash of everything (should be) in the caster's shadow map and any parameters about it
	// - light params
	// - every in-range shadow-casting object
	u64 content_hash = 0;
};

struct TranslucentDraw {
	unsigned object_index = 0; // first instance
	unsigned page_index = 0;
	unsigned index_count = 0;
	unsigned first_index = 0;
};

struct FrameParams {
	gfx::RingBuffer *upload = nullptr;

	const FramePacket *packet = nullptr;

	u64 frame_number = 0;

	float dt = 0.f;
	float elapsed = 0.f;

	Camera camera;

	gfx::Alloc frame_data;

	gfx::Alloc mesh_table;
	gfx::Alloc material_table;

	unsigned page_count = 0;
	gfx::Alloc page_table_buffer;
	gfx::ResourceKey page_index_buffers[RENDER_SCENE_MAX_GEOMETRY_PAGES];
	gfx::IndexType page_index_types[RENDER_SCENE_MAX_GEOMETRY_PAGES];

	unsigned object_count = 0;
	gfx::Alloc object_buffer;

	unsigned light_count = 0;
	gfx::Alloc light_buffer;

	gfx::Alloc skinning_palette_buffer;

	unsigned shadow_caster_count = 0;
	ShadowCaster shadow_casters[RENDER_FRAME_PARAMS_MAX_SHADOW_CASTERS];
	gfx::Alloc shadow_caster_table;

	Vector<TranslucentDraw> translucent_draws; // sorted BTF

	gfx::ResourceKey brdf_lut;
	gfx::ResourceKey irradiance_fallback_cubemap;
	gfx::ResourceKey prefilter_cubemap;

	const Mesh *skybox_mesh = nullptr;

	gfx::ResourceKey cubemap_capture_transform_buffer;

	gfx::ResourceKey linear_sampler;
	gfx::ResourceKey nearest_sampler;
	gfx::ResourceKey material_sampler;
	gfx::ResourceKey shadow_compare_sampler;

	// to prevent a shader hot-swap from changing mid-frame
	bool shaders_ready = false;

	gfx::ResourceKey debug_line_shader;
	gfx::ResourceKey depth_prepass_shader;
	gfx::ResourceKey forward_shader;
	gfx::ResourceKey shadow_shader;
	gfx::ResourceKey shadow_masked_shader;
	gfx::ResourceKey shadow_cull_shader;
	gfx::ResourceKey cull_frustum_shader;
	gfx::ResourceKey skybox_shader;
	gfx::ResourceKey tonemapping_shader;
	gfx::ResourceKey brdf_lut_generation_shader;
	gfx::ResourceKey hdr_to_cubemap_shader;
	gfx::ResourceKey irradiance_cubemap_gen_shader;
	gfx::ResourceKey prefilter_cubemap_gen_shader;

	// rebuilds everything in place so the vectors keep their capacity.
	void Build(System *system, gfx::RingBuffer *upload, const FramePacket *packet);

	// shared resources only (no packet), used for one-off bakes.
	void BuildShared(System *system);

	bool ResolveShaders(const System *system);

	void RecordIndirect(
		const gfx::CmdBuffer &cmd,
		gfx::ResourceKey indirect_buffer,
		gfx::ResourceKey count_buffer
	) const;

	void RecordIndirectStream(
		const gfx::CmdBuffer &cmd,
		gfx::ResourceKey indirect_buffer,
		gfx::ResourceKey count_buffer,
		u64 stream_index
	) const;

	void RecordTranslucent(const gfx::CmdBuffer &cmd) const;

private:
	void UploadSceneTables(const FramePacket &packet, gfx::RingBuffer *upload);
	void UploadObjects(const FramePacket &packet, gfx::RingBuffer *upload);
	void UploadLights(const FramePacket &packet, gfx::RingBuffer *upload);
	void UploadFrameData(const FramePacket &packet, gfx::RingBuffer *upload);
	void BuildTranslucentList(const FramePacket &packet);
};

}

#endif // RENDER_FRAME_H
