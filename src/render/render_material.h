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
	G_ResourceKey albedo_texture;
	G_ResourceKey normal_texture;
	G_ResourceKey emissive_texture;
	G_ResourceKey metallic_roughness_texture; // g = roughness, b = metallic
	G_ResourceKey occlusion_texture; // r channel only
	
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
	G_ResourceKey transmission_texture;
	G_ResourceKey thickness_texture;

	f32 transmission_factor;
	f32 thickness_factor;

	v3  attenuation_colour;
	f32 attenuation_distance;

	/*
	 * Overrides the usual F0 value which is usually around .04
	 * for most materials.
	 */
	G_ResourceKey specular_texture; // r = specular
	G_ResourceKey specular_colour_texture;

	f32 specular_factor;
	v3  specular_colour_factor;

	/*
	 * Some materials have another "layer" over the main material
	 * that can't be simulated using just roughness called a clearcoat.
	 * e.g: car paint, varnished wood, wet surfaces, etc...
	 */
	G_ResourceKey clearcoat_texture;
	G_ResourceKey clearcoat_roughness_texture;

	f32 clearcoat_factor;
	f32 clearcoat_roughness_factor;

	/*
	 * Retroreflectiveness, i.e.: light scattering back towards
	 * the light source at grazing angles. Typically results
	 * in the material being darker when you look at it head on.
	 * e.g: cloth & fabrics (VELVET ESPECIALLY).
	 */
	G_ResourceKey sheen_colour_texture;
	G_ResourceKey sheen_roughness_texture;

	v3  sheen_colour_factor;
	f32 sheen_roughness_factor;

	/*
	 * Thin-film interference. Lowk just looks cool af. Soap, pearl
	 * CD's, butterfly wings, etc...
	 */
	G_ResourceKey iridescence_texture;
	G_ResourceKey iridescence_thickness_texture;

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

internal G_ResourceKey R_MaterialResolveAssetTexture(A_Handle handle);

internal R_Material R_MaterialDefault(void);
internal R_Material R_MaterialFromAsset(const A_ModelMaterial *source);

#endif // RENDER_MATERIAL_H
