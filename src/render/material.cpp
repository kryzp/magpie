#include "render/material.h"
#include "asset/asset_system.h"

gfx::ResourceKey rnd::Material::ResolveAssetTexture(ast::Handle handle)
{
	if (handle.IsNull())
		return gfx::ResourceKey::Null();

	return ast::AssetSystem::Get()->GetOrFallback(handle)->texture.key;
}

rnd::Material rnd::Material::Default()
{
	Material mat;

	mat.albedo_factor = Vec4(1.f, 1.f, 1.f, 1.f);
	mat.normal_scale = 1.f;
	mat.metallic_factor = 1.f;
	mat.roughness_factor = 1.f;
	mat.emissive_factor = Vec3(0.f, 0.f, 0.f);
	mat.emissive_intensity = 1.f;
	mat.occlusion_intensity = 1.f;

	mat.ior = 1.5f;

	mat.transmission_factor = 0.f;
	mat.thickness_factor = 0.f;
	mat.attenuation_colour = Vec3(1.f, 1.f, 1.f);
	mat.attenuation_distance = maths::MAX_FLOAT; // gltf spec.

	mat.specular_factor = 1.f;
	mat.specular_colour_factor = Vec3(1.f, 1.f, 1.f);

	mat.clearcoat_factor = 0.f;
	mat.clearcoat_roughness_factor = 0.f;

	mat.sheen_colour_factor = Vec3(0.f, 0.f, 0.f);
	mat.sheen_roughness_factor = 0.f;

	mat.iridescence_factor = 0.f;
	mat.iridescence_ior = 1.3f;
	mat.iridescence_thickness_min_nanometers = 100.f;
	mat.iridescence_thickness_max_nanometers = 400.f;

	mat.double_sided = false;
	mat.unlit = false;

	mat.alpha_cutoff = 0.5f;
	mat.alpha_mode = ast::AlphaMode_Opaque;

	mat.gloss_in_specular_alpha = false;

	mat.reflection_mode = ast::ReflectionMode_Default;
	mat.reflection_plane = Vec4(0.f, 0.f, 0.f, 0.f);

	return mat;
}

// js slime me out gng
// gurt: yo
rnd::Material rnd::Material::FromAsset(const ast::ModelMaterial &source)
{
	Material mat = Default();

	mat.name = source.name;

	mat.albedo_texture = ResolveAssetTexture(source.albedo_texture);
	mat.normal_texture = ResolveAssetTexture(source.normal_texture);
	mat.emissive_texture = ResolveAssetTexture(source.emissive_texture);
	mat.metallic_roughness_texture = ResolveAssetTexture(source.metallic_roughness_texture);
	mat.occlusion_texture = ResolveAssetTexture(source.occlusion_texture);

	mat.albedo_factor = source.albedo_factor;
	mat.normal_scale = source.normal_scale;
	mat.metallic_factor = source.metallic_factor;
	mat.roughness_factor = source.roughness_factor;
	mat.emissive_factor = source.emissive_factor;
	mat.emissive_intensity = source.emissive_intensity;
	mat.occlusion_intensity = source.occlusion_intensity;

	mat.ior = source.ior;

	mat.transmission_texture = ResolveAssetTexture(source.transmission_texture);
	mat.thickness_texture = ResolveAssetTexture(source.thickness_texture);
	mat.transmission_factor = source.transmission_factor;
	mat.thickness_factor = source.thickness_factor;
	mat.attenuation_colour = source.attenuation_colour;
	mat.attenuation_distance = source.attenuation_distance;

	mat.specular_texture = ResolveAssetTexture(source.specular_texture);
	mat.specular_colour_texture = ResolveAssetTexture(source.specular_colour_texture);
	mat.specular_factor = source.specular_factor;
	mat.specular_colour_factor = source.specular_colour_factor;

	mat.clearcoat_texture = ResolveAssetTexture(source.clearcoat_texture);
	mat.clearcoat_roughness_texture = ResolveAssetTexture(source.clearcoat_roughness_texture);
	mat.clearcoat_factor = source.clearcoat_factor;
	mat.clearcoat_roughness_factor = source.clearcoat_roughness_factor;

	mat.sheen_colour_texture = ResolveAssetTexture(source.sheen_colour_texture);
	mat.sheen_roughness_texture = ResolveAssetTexture(source.sheen_roughness_texture);
	mat.sheen_colour_factor = source.sheen_colour_factor;
	mat.sheen_roughness_factor = source.sheen_roughness_factor;

	mat.iridescence_texture = ResolveAssetTexture(source.iridescence_texture);
	mat.iridescence_thickness_texture = ResolveAssetTexture(source.iridescence_thickness_texture);
	mat.iridescence_factor = source.iridescence_factor;
	mat.iridescence_ior = source.iridescence_ior;
	mat.iridescence_thickness_min_nanometers = source.iridescence_thickness_min_nanometers;
	mat.iridescence_thickness_max_nanometers = source.iridescence_thickness_max_nanometers;

	mat.double_sided = source.double_sided;

	mat.unlit = source.unlit;

	mat.alpha_cutoff = source.alpha_cutoff;
	mat.alpha_mode = source.alpha_mode;

	mat.gloss_in_specular_alpha = source.gloss_in_specular_alpha;

	mat.reflection_mode = source.reflection_mode;
	mat.reflection_plane = source.reflection_plane;

	return mat;
}
