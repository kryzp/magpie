#ifndef ASSET_MATERIAL_H
#define ASSET_MATERIAL_H

typedef enum AST_AlphaMode
{
	AST_AlphaMode_Opaque = 0,  // -> deferred
	AST_AlphaMode_Mask   = 1,  // -> deferred + discard below threshold
	AST_AlphaMode_Blend  = 2,  // -> forward transparency, back-to-front
	AST_AlphaMode_COUNT
}
AST_AlphaMode;

typedef enum AST_ReflectionMode
{
	AST_ReflectionMode_Default,  // -> ssr + probes
	AST_ReflectionMode_Planar,   // -> full scene re-render
	AST_ReflectionMode_None,     // -> no reflections at all
	AST_ReflectionMode_COUNT,
}
AST_ReflectionMode;

typedef struct AST_ModelMaterial AST_ModelMaterial;
struct AST_ModelMaterial
{
	/*
	 * Standard Metallic-Roughness PBR crap.
	 */
	AST_Handle albedo_texture;
	AST_Handle normal_texture;
	AST_Handle emissive_texture;
	AST_Handle metallic_roughness_texture; // g = roughness, b = metallic
	AST_Handle occlusion_texture; // r channel only
	
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
	AST_Handle transmission_texture;
	AST_Handle thickness_texture;

	f32 transmission_factor;
	f32 thickness_factor;

	v3  attenuation_colour;
	f32 attenuation_distance;

	/*
	 * Overrides the usual F0 value which is usually around .04
	 * for most materials.
	 */
	AST_Handle specular_texture; // r = specular
	AST_Handle specular_colour_texture;

	f32 specular_factor;
	v3  specular_colour_factor;

	/*
	 * Some materials have another "layer" over the main material
	 * that can't be simulated using just roughness called a clearcoat.
	 * e.g: car paint, varnished wood, wet surfaces, etc...
	 */
	AST_Handle clearcoat_texture;
	AST_Handle clearcoat_roughness_texture;

	f32 clearcoat_factor;
	f32 clearcoat_roughness_factor;

	/*
	 * Retroreflectiveness, i.e.: light scattering back towards
	 * the light source at grazing angles. Typically results
	 * in the material being darker when you look at it head on.
	 * e.g: cloth & fabrics (VELVET ESPECIALLY).
	 */
	AST_Handle sheen_colour_texture;
	AST_Handle sheen_roughness_texture;

	v3  sheen_colour_factor;
	f32 sheen_roughness_factor;

	/*
	 * Thin-film interference. Lowk just looks cool af. Soap, pearl
	 * CD's, butterfly wings, etc...
	 */
	AST_Handle iridescence_texture;
	AST_Handle iridescence_thickness_texture;

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

#endif // ASSET_MATERIAL_H
