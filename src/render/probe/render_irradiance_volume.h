#ifndef RENDER_IRRADIANCE_VOLUME_H
#define RENDER_IRRADIANCE_VOLUME_H

/*
 * TODO THIS IS EXPERIMENTAL AND COMPLETE
 * GARBAGE CODE-WISE.
 */

/*
 * Ray traced irradiance probing using L2 harmonics.
 *
 * - "An Efficient Representation for Irradiance Environment Maps"
 * - "Dynamic Diffuse static Illumination with Ray-Traced Irradiance Fields"
 */

#define R_IRRADIANCE_RAYS_PER_PROBE 64

// This file shouldn't really be called this I should rename it.
// Originally tried implementing a 3D texture irradiance volume but that was
// too hard :( and I wanna see results because I'm an impatient gremlin.

/*
 * Irradiance harmonics are pretty complicated on the maths side but the general idea is
 * that since traditional irradiance cubemaps tend to be very "smooth" for lack of a better
 * term we can compress them into just their harmonic components (in the same way a fourier
 * transform might approximate a wave function using just the first N frequencies).
 *
 * We use the first 9 basis functions:
 *   [0] l=0, m=0  (constant)
 *   [1] l=1, m=-1 (Y)
 *   [2] l=1, m=0  (Z)
 *   [3] l=1, m=1  (X)
 *   [4] l=2, m=-2 (XY)
 *   [5] l=2, m=-1 (YZ)
 *   [6] l=2, m=0  (3Z^2 - 1)
 *   [7] l=2, m=1  (XZ)
 *   [8] l=2, m=2  (X^2 - Y^2)
 */

#define R_SH_COEFFICIENT_COUNT 9

typedef struct R_GPU_ProbeSH R_GPU_ProbeSH;
struct R_GPU_ProbeSH
{
	v4 coefficients[R_SH_COEFFICIENT_COUNT]; // [r, g, b, 0]
};

typedef struct R_GPU_ProbeGridInfo R_GPU_ProbeGridInfo;
struct R_GPU_ProbeGridInfo
{
	v3  grid_min;
	f32 _pad0;

	v3  grid_max;
	f32 _pad1;

	v3  cell_size;
	f32 _pad2;

	u32 nx;
	u32 ny;
	u32 nz;
	u32 ntotal;
};

typedef struct R_IrradianceVolume R_IrradianceVolume;
struct R_IrradianceVolume
{
	G_Device *device;
	A_Assets *assets;
	
	LOG_Channel log_channel;

	v3 grid_min;
	v3 grid_max;

	u32 nx;
	u32 ny;
	u32 nz;
	u32 ntotal;

	G_BufferKey sh_buffer;
	G_BufferKey grid_info_buffer;
	
	A_Handle bake_shader_handle;

	const R_Mesh *skybox_mesh;
	G_TextureViewKey environment_view;
	G_SamplerKey linear_sampler;

	G_AccelStructKey tlas;
	G_AccelStructKey blas_per_page[32]; // we need a blas per geometry page in the scene
	u32 blas_count;

	b32 is_baked;
};

static void R_IrradianceVolumeInit(R_IrradianceVolume *vol,
								   G_Device *device, A_Assets *assets,
								   LOG_Channel log_channel,
								   v3 grid_min, v3 grid_max,
								   u32 nx, u32 ny, u32 nz,
								   const R_Mesh *skybox_mesh,
								   G_TextureViewKey environment_view,
								   G_SamplerKey linear_sampler);

static void R_IrradianceVolumeDestroy(R_IrradianceVolume *vol);

static void R_IrradianceVolumeBuildAccelStructs(R_IrradianceVolume *vol, const R_Scene *scene);
static void R_IrradianceVolumeBake(R_IrradianceVolume *vol, const R_Scene *scene);

static void R_IrradianceVolumeDebug(const R_IrradianceVolume *vol);

static G_BufferKey R_IrradianceVolumeGetSHBuffer(const R_IrradianceVolume *vol);
static G_BufferKey R_IrradianceVolumeGetGridInfoBuffer(const R_IrradianceVolume *vol);
static b32 R_IrradianceVolumeIsBaked(const R_IrradianceVolume *vol);

#endif // RENDER_IRRADIANCE_VOLUME_H
