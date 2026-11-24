#ifndef ASSET_MATERIAL_H
#define ASSET_MATERIAL_H

#include "asset/handle.h"
#include "core/maths.h"
#include "container/string.h"

namespace ast {

enum AlphaMode {
	AlphaMode_Opaque = 0, // -> deferred
	AlphaMode_Mask = 1,   // -> deferred + discard below threshold
	AlphaMode_Blend = 2,  // -> forward transparency, back-to-front
	AlphaMode_MaxEnum
};

enum ReflectionMode {
	ReflectionMode_Default, // -> ssr + probes
	ReflectionMode_Planar,  // -> full scene re-render
	ReflectionMode_None,    // -> no reflections at all
	ReflectionMode_MaxEnum
};

// inits to default material per glTF spec.
struct ModelMaterial {
	String name;

	/*
	 * Standard Metallic-Roughness PBR crap.
	 */
	Handle albedo_texture;
	Handle normal_texture;
	Handle emissive_texture;
	Handle metallic_roughness_texture; // g = roughness, b = metallic
	Handle occlusion_texture;          // r channel only

	Vec4 albedo_factor = Vec4(1.f, 1.f, 1.f, 1.f);
	float normal_scale = 1.f;
	float metallic_factor = 1.f;
	float roughness_factor = 1.f;
	Vec3 emissive_factor = Vec3(0.f);
	float emissive_intensity = 1.f;
	float occlusion_intensity = 1.f;

	float ior = 1.5f; // index of refraction (eta)

	/*
	 * Light can pass through objects, and transmission tells
	 * us "how much" of the light makes its way through.
	 */
	Handle transmission_texture;
	Handle thickness_texture;

	float transmission_factor = 0.f;
	float thickness_factor = 0.f;

	Vec3 attenuation_colour = Vec3(1.f);
	float attenuation_distance = maths::MAX_FLOAT; // gltf spec.

	/*
	 * Overrides the usual F0 value which is usually around .04
	 * for most materials.
	 */
	Handle specular_texture; // r = specular
	Handle specular_colour_texture;

	float specular_factor = 1.f;
	Vec3 specular_colour_factor = Vec3(1.f);

	/*
	 * Some materials have another "layer" over the main material
	 * that can't be simulated using just roughness called a clearcoat.
	 * e.g: car paint, varnished wood, wet surfaces, etc...
	 */
	Handle clearcoat_texture;
	Handle clearcoat_roughness_texture;

	float clearcoat_factor = 0.f;
	float clearcoat_roughness_factor = 0.f;

	/*
	 * Retroreflectiveness, i.e.: light scattering back towards
	 * the light source at grazing angles. Typically results
	 * in the material being darker when you look at it head on.
	 * e.g: cloth & fabrics (VELVET ESPECIALLY).
	 */
	Handle sheen_colour_texture;
	Handle sheen_roughness_texture;

	Vec3 sheen_colour_factor = Vec3(0.f);
	float sheen_roughness_factor = 0.f;

	/*
	 * Thin-film interference. Lowk just looks cool af.
	 * Soap, pearls, CD's, butterfly wings, etc...
	 */
	Handle iridescence_texture;
	Handle iridescence_thickness_texture;

	float iridescence_factor = 0.f;
	float iridescence_ior = 1.3f;
	float iridescence_thickness_min_nanometers = 100.f;
	float iridescence_thickness_max_nanometers = 400.f;

	/*
	 * Etc.
	 */
	bool double_sided = false;
	bool unlit = false;

	float alpha_cutoff = 0.5f;
	AlphaMode alpha_mode = AlphaMode_Opaque;

	bool gloss_in_specular_alpha = false;

	// note: not part of GLTF spec! just engine flags (not yet implemented tho)
	ReflectionMode reflection_mode = ReflectionMode_Default;
	Vec4 reflection_plane;
};

}

#endif // ASSET_MATERIAL_H
