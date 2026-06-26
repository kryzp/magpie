#ifndef RENDER_MATERIAL_H
#define RENDER_MATERIAL_H

// I feel like shit about this because I really shouldn't be
// just copy pasting but it's really helpful to have a seperate
// R_Material that lives seperately from assets.
typedef struct R_Material R_Material;
struct R_Material
{
	String8 name;
	
	/*
	 * Standard Metallic-Roughness PBR crap.
	 */
	G_TextureKey albedo_texture;
	G_TextureKey normal_texture;
	G_TextureKey emissive_texture;
	G_TextureKey metallic_roughness_texture; // g = roughness, b = metallic
	G_TextureKey occlusion_texture; // r channel only
	
	v4  albedo_factor;
	f32 normal_scale;
	f32 metallic_factor;
	f32 roughness_factor;
	v3  emissive_factor;
	f32 emissive_intensity;
	f32 occlusion_intensity;

	f32 ior; // index of refraction (eta)

	/*
	 * Light can pass through objects, and transmission tells
	 * us "how much" of the light makes its way through.
	 */
	G_TextureKey transmission_texture;
	G_TextureKey thickness_texture;

	f32 transmission_factor;
	f32 thickness_factor;

	v3  attenuation_colour;
	f32 attenuation_distance;

	/*
	 * Overrides the usual F0 value which is usually around .04
	 * for most materials.
	 */
	G_TextureKey specular_texture; // r = specular
	G_TextureKey specular_colour_texture;

	f32 specular_factor;
	v3  specular_colour_factor;

	/*
	 * Some materials have another "layer" over the main material
	 * that can't be simulated using just roughness called a clearcoat.
	 * e.g: car paint, varnished wood, wet surfaces, etc...
	 */
	G_TextureKey clearcoat_texture;
	G_TextureKey clearcoat_roughness_texture;

	f32 clearcoat_factor;
	f32 clearcoat_roughness_factor;

	/*
	 * Retroreflectiveness, i.e.: light scattering back towards
	 * the light source at grazing angles. Typically results
	 * in the material being darker when you look at it head on.
	 * e.g: cloth & fabrics (VELVET ESPECIALLY).
	 */
	G_TextureKey sheen_colour_texture;
	G_TextureKey sheen_roughness_texture;

	v3  sheen_colour_factor;
	f32 sheen_roughness_factor;

	/*
	 * Thin-film interference. Lowk just looks cool af. Soap, pearl
	 * CD's, butterfly wings, etc...
	 */
	G_TextureKey iridescence_texture;
	G_TextureKey iridescence_thickness_texture;

	f32 iridescence_factor;
	f32 iridescence_ior;
	f32 iridescence_thickness_min_nanometers;
	f32 iridescence_thickness_max_nanometers;

	/*
	 * Etc.
	 */
	b32 double_sided;
	b32 unlit;
	
	f32 alpha_cutoff;
	A_AlphaMode alpha_mode;

	// note: not part of GLTF spec! just engine flags (not yet implemented tho)
	A_ReflectionMode reflection_mode;
	v4 reflection_plane;
};

static G_TextureKey R_MaterialResolveAssetTexture(A_Assets *assets, A_Handle handle);

static R_Material R_MaterialDefault(void);
static R_Material R_MaterialFromAsset(const A_ModelMaterial *source, A_Assets *assets);

#endif // RENDER_MATERIAL_H
