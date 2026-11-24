#ifndef RENDER_MATERIAL_H
#define RENDER_MATERIAL_H

#include "core/types.h"
#include "core/maths.h"
#include "graphics/resource_key.h"
#include "asset/handle.h"
#include "asset/types.h"

#include "container/string.h"

namespace rnd {

// I feel like shit about this because I really shouldn't be
// just copy pasting but it's really helpful to have a seperate
// rnd::Material that lives seperately from assets.
struct Material {
	String name;

	/*
	 * Standard Metallic-Roughness PBR crap.
	 */
	gfx::ResourceKey albedo_texture;
	gfx::ResourceKey normal_texture;
	gfx::ResourceKey emissive_texture;
	gfx::ResourceKey metallic_roughness_texture; // g = roughness, b = metallic
	gfx::ResourceKey occlusion_texture;          // r channel only

	Vec4 albedo_factor;
	float normal_scale = 0.f;
	float metallic_factor = 0.f;
	float roughness_factor = 0.f;
	Vec3 emissive_factor;
	float emissive_intensity = 0.f;
	float occlusion_intensity = 0.f;

	float ior = 0.f; // index of refraction (eta)

	/*
	 * Light can pass through objects, and transmission tells
	 * us "how much" of the light makes its way through.
	 */
	gfx::ResourceKey transmission_texture;
	gfx::ResourceKey thickness_texture;

	float transmission_factor = 0.f;
	float thickness_factor = 0.f;

	Vec3 attenuation_colour;
	float attenuation_distance = 0.f;

	/*
	 * Overrides the usual F0 value which is usually around .04
	 * for most materials.
	 */
	gfx::ResourceKey specular_texture; // r = specular
	gfx::ResourceKey specular_colour_texture;

	float specular_factor = 0.f;
	Vec3 specular_colour_factor;

	/*
	 * Some materials have another "layer" over the main material
	 * that can't be simulated using just roughness called a clearcoat.
	 * e.g: car paint, varnished wood, wet surfaces, etc...
	 */
	gfx::ResourceKey clearcoat_texture;
	gfx::ResourceKey clearcoat_roughness_texture;

	float clearcoat_factor = 0.f;
	float clearcoat_roughness_factor = 0.f;

	/*
	 * Retroreflectiveness, i.e.: light scattering back towards
	 * the light source at grazing angles. Typically results
	 * in the material being darker when you look at it head on.
	 * e.g: cloth & fabrics (VELVET ESPECIALLY).
	 */
	gfx::ResourceKey sheen_colour_texture;
	gfx::ResourceKey sheen_roughness_texture;

	Vec3 sheen_colour_factor;
	float sheen_roughness_factor = 0.f;

	/*
	 * Thin-film interference. Lowk just looks cool af. Soap, pearl
	 * CD's, butterfly wings, etc...
	 */
	gfx::ResourceKey iridescence_texture;
	gfx::ResourceKey iridescence_thickness_texture;

	float iridescence_factor = 0.f;
	float iridescence_ior = 0.f;
	float iridescence_thickness_min_nanometers = 0.f;
	float iridescence_thickness_max_nanometers = 0.f;

	/*
	 * Etc.
	 */
	bool double_sided = false;
	bool unlit = false;

	float alpha_cutoff = 0.f;
	ast::AlphaMode alpha_mode = ast::AlphaMode_Opaque;

	bool gloss_in_specular_alpha = false;

	// note: not part of GLTF spec! just engine flags (not yet implemented tho)
	ast::ReflectionMode reflection_mode = ast::ReflectionMode_Default;
	Vec4 reflection_plane;

	static gfx::ResourceKey ResolveAssetTexture(ast::Handle handle);

	static Material Default();
	static Material FromAsset(const ast::ModelMaterial &source);
};

}

#endif // RENDER_MATERIAL_H
