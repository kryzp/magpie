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
	GFX_TextureKey albedo_texture;
	GFX_TextureKey normal_texture;
	GFX_TextureKey emissive_texture;
	GFX_TextureKey metallic_roughness_texture; // g = roughness, b = metallic
	GFX_TextureKey occlusion_texture; // r channel only
	
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
	GFX_TextureKey transmission_texture;
	GFX_TextureKey thickness_texture;

	f32 transmission_factor;
	f32 thickness_factor;

	v3  attenuation_colour;
	f32 attenuation_distance;

	/*
	 * Overrides the usual F0 value which is usually around .04
	 * for most materials.
	 */
	GFX_TextureKey specular_texture; // r = specular
	GFX_TextureKey specular_colour_texture;

	f32 specular_factor;
	v3  specular_colour_factor;

	/*
	 * Some materials have another "layer" over the main material
	 * that can't be simulated using just roughness called a clearcoat.
	 * e.g: car paint, varnished wood, wet surfaces, etc...
	 */
	GFX_TextureKey clearcoat_texture;
	GFX_TextureKey clearcoat_roughness_texture;

	f32 clearcoat_factor;
	f32 clearcoat_roughness_factor;

	/*
	 * Retroreflectiveness, i.e.: light scattering back towards
	 * the light source at grazing angles. Typically results
	 * in the material being darker when you look at it head on.
	 * e.g: cloth & fabrics (VELVET ESPECIALLY).
	 */
	GFX_TextureKey sheen_colour_texture;
	GFX_TextureKey sheen_roughness_texture;

	v3  sheen_colour_factor;
	f32 sheen_roughness_factor;

	/*
	 * Thin-film interference. Lowk just looks cool af. Soap, pearl
	 * CD's, butterfly wings, etc...
	 */
	GFX_TextureKey iridescence_texture;
	GFX_TextureKey iridescence_thickness_texture;

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
	AST_AlphaMode alpha_mode;

	// note: not part of GLTF spec! just engine flags (not yet implemented tho)
	AST_ReflectionMode reflection_mode;
	v4 reflection_plane;
};

internal GFX_TextureKey R_MaterialResolveAssetTexture(AST_Assets *assets, AST_Handle handle);

internal R_Material R_MaterialDefault(void);
internal R_Material R_MaterialFromAsset(const AST_ModelMaterial *source, AST_Assets *assets);

#endif // RENDER_MATERIAL_H
