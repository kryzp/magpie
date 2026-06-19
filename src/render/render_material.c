
internal G_TextureKey
R_MaterialResolveAssetTexture(A_Registry *assets, A_Handle handle)
{
	if (!A_IsValid(assets, handle) || !A_IsLoaded(assets, handle))
		return G_TextureKeyNull();

	A_Asset *asset = A_GetNow(assets, handle);
	return asset->texture.key;
}

internal R_Material
R_MaterialDefault(void)
{
	R_Material mat = {0};

	mat.albedo_factor                        = v4(1.f, 1.f, 1.f, 1.f);
	mat.normal_scale                         = 1.f;
	mat.metallic_factor                      = 1.f;
	mat.roughness_factor                     = 1.f;
	mat.emissive_factor                      = v3(0.f, 0.f, 0.f);
	mat.emissive_intensity                   = 1.f;
	mat.occlusion_intensity                  = 1.f;

	mat.ior                                  = 1.5f;

	mat.transmission_factor                  = 0.f;
	mat.thickness_factor                     = 0.f;
	mat.attenuation_colour                   = v3(1.f, 1.f, 1.f);
	mat.attenuation_distance                 = MATH_MAX_F32; // gltf spec.

	mat.specular_factor                      = 1.f;
	mat.specular_colour_factor               = v3(1.f, 1.f, 1.f);

	mat.clearcoat_factor                     = 0.f;
	mat.clearcoat_roughness_factor           = 0.f;

	mat.sheen_colour_factor                  = v3(0.f, 0.f, 0.f);
	mat.sheen_roughness_factor               = 0.f;

	mat.iridescence_factor                   = 0.f;
	mat.iridescence_ior                      = 1.3f;
	mat.iridescence_thickness_min_nanometers = 100.f;
	mat.iridescence_thickness_max_nanometers = 400.f;

	mat.alpha_mode                           = A_AlphaMode_Opaque;
	mat.alpha_cutoff                         = 0.5f;
	mat.double_sided                         = false;
	mat.unlit                                = false;

	mat.reflection_mode                      = A_ReflectionMode_Default;
	mat.reflection_plane                     = v4(0.f, 0.f, 0.f, 0.f);
	
	return mat;
}

// js slime me out gng
internal R_Material
R_MaterialFromAsset(const A_ModelMaterial *source, A_Registry *assets)
{
	R_Material mat = R_MaterialDefault();

	mat.albedo_texture                       = R_MaterialResolveAssetTexture(assets, source->albedo_texture);
	mat.normal_texture                       = R_MaterialResolveAssetTexture(assets, source->normal_texture);
	mat.emissive_texture                     = R_MaterialResolveAssetTexture(assets, source->emissive_texture);
	mat.metallic_roughness_texture           = R_MaterialResolveAssetTexture(assets, source->metallic_roughness_texture);
	mat.occlusion_texture                    = R_MaterialResolveAssetTexture(assets, source->occlusion_texture);
	
	mat.albedo_factor                        = source->albedo_factor;
	mat.normal_scale                         = source->normal_scale;
	mat.metallic_factor                      = source->metallic_factor;
	mat.roughness_factor                     = source->roughness_factor;
	mat.emissive_factor                      = source->emissive_factor;
	mat.emissive_intensity                   = source->emissive_intensity;
	mat.occlusion_intensity                  = source->occlusion_intensity;

	mat.ior                                  = source->ior;

	mat.transmission_texture                 = R_MaterialResolveAssetTexture(assets, source->transmission_texture);
	mat.thickness_texture                    = R_MaterialResolveAssetTexture(assets, source->thickness_texture);
	mat.transmission_factor                  = source->transmission_factor;
	mat.thickness_factor                     = source->thickness_factor;
	mat.attenuation_colour                   = source->attenuation_colour;
	mat.attenuation_distance                 = source->attenuation_distance;

	mat.specular_texture                     = R_MaterialResolveAssetTexture(assets, source->specular_texture);
	mat.specular_colour_texture              = R_MaterialResolveAssetTexture(assets, source->specular_colour_texture);
	mat.specular_factor                      = source->specular_factor;
	mat.specular_colour_factor               = source->specular_colour_factor;

	mat.clearcoat_texture                    = R_MaterialResolveAssetTexture(assets, source->clearcoat_texture);
	mat.clearcoat_roughness_texture          = R_MaterialResolveAssetTexture(assets, source->clearcoat_roughness_texture);
	mat.clearcoat_factor                     = source->clearcoat_factor;
	mat.clearcoat_roughness_factor           = source->clearcoat_roughness_factor;

	mat.sheen_colour_texture                 = R_MaterialResolveAssetTexture(assets, source->sheen_colour_texture);
	mat.sheen_roughness_texture              = R_MaterialResolveAssetTexture(assets, source->sheen_roughness_texture);
	mat.sheen_colour_factor                  = source->sheen_colour_factor;
	mat.sheen_roughness_factor               = source->sheen_roughness_factor;

	mat.iridescence_texture                  = R_MaterialResolveAssetTexture(assets, source->iridescence_texture);
	mat.iridescence_thickness_texture        = R_MaterialResolveAssetTexture(assets, source->iridescence_thickness_texture);
	mat.iridescence_factor                   = source->iridescence_factor;
	mat.iridescence_ior                      = source->iridescence_ior;
	mat.iridescence_thickness_min_nanometers = source->iridescence_thickness_min_nanometers;
	mat.iridescence_thickness_max_nanometers = source->iridescence_thickness_max_nanometers;

	mat.double_sided                         = source->double_sided;

	mat.unlit                                = source->unlit;

	mat.alpha_cutoff                         = source->alpha_cutoff;
	mat.alpha_mode                           = source->alpha_mode;

	mat.reflection_mode                      = source->reflection_mode;
	mat.reflection_plane                     = source->reflection_plane;
	
	return mat;
}
