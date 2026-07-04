
static void R_MaterialRegistryInit(R_MaterialRegistry *r, G_Device *device, A_Assets *assets, LOG_Channel log_channel)
{
	r->device = device;
	r->assets = assets;

	r->log_channel = log_channel;

	for (u32 i = 0; i < ArraySize(r->material_slots); i++)
		r->material_slots[i].generation = 1;

	for (i32 i = ArraySize(r->material_slots) - 1; i > 0; i--)
		r->material_free_list[r->material_free_count++] = i - 1;

	G_BufferAllocInfo material_buffer_alloc_info = {0};
	material_buffer_alloc_info.usage = VK_BUFFER_USAGE_2_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_2_TRANSFER_DST_BIT;
	material_buffer_alloc_info.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
	material_buffer_alloc_info.size = sizeof(R_GPU_Material) * ArraySize(r->material_slots);
	
	r->material_buffer = G_DeviceBufferAlloc(r->device, &material_buffer_alloc_info);
	
	r->material_buffer_dirty = true;
}

static void R_MaterialRegistryDestroy(R_MaterialRegistry *r)
{
	G_DeviceBufferDestroy(r->device, r->material_buffer);
}

static R_SceneHandle R_MaterialRegistryAddMaterial(R_MaterialRegistry *r, const R_Material *material)
{
	DebugLogAssert(r->log_channel,
				   r->material_free_count > 0,
				   "Ran out of free material slots.");

	r->material_free_count--;
	u32 slot_index = r->material_free_list[r->material_free_count];

	R_MaterialSlot *slot = &r->material_slots[slot_index];

	slot->source = *material;
	slot->active = true;

	R_MaterialRegistryBakeIntoGPU(r, material, &r->material_gpus[slot_index]);
	r->material_buffer_dirty = true;

	r->material_count++;

	R_SceneHandle handle = {0};
	handle.index = slot_index;
	handle.generation = slot->generation;

	return handle;
}

static R_SceneHandle R_MaterialRegistryAddFromAssets(R_MaterialRegistry *r, const A_ModelMaterial *source)
{
	R_Material material = R_MaterialFromAsset(source, r->assets);
	return R_MaterialRegistryAddMaterial(r, &material);
}

static void R_MaterialRegistryUpdate(R_MaterialRegistry *r, R_SceneHandle handle, const R_Material *material)
{
	if (handle.index >= ArraySize(r->material_slots))
		return;

	R_MaterialSlot *slot = &r->material_slots[handle.index];

	if (!slot->active || slot->generation != handle.generation)
		return;

	slot->source = *material;

	R_MaterialRegistryBakeIntoGPU(r, material, &r->material_gpus[handle.index]);
	r->material_buffer_dirty = true;
}

static void R_MaterialRegistryDisposeOfMaterial(R_MaterialRegistry *r, R_SceneHandle handle)
{
	if (handle.index >= ArraySize(r->material_slots))
		return;

	R_MaterialSlot *slot = &r->material_slots[handle.index];

	if (!slot->active || slot->generation != handle.generation)
		return;

	slot->active = false;
	slot->generation++;

	MemZeroStruct(&r->material_gpus[handle.index]);
	r->material_buffer_dirty = true;

	r->material_free_list[r->material_free_count++] = handle.index;
	r->material_count--;
}

static u32 R_MaterialRegistryCountOfMaterials(const R_MaterialRegistry *r)
{
	return r->material_count;
}

static const R_Material *R_MaterialRegistryGetSource(const R_MaterialRegistry *r, R_SceneHandle handle)
{
	if (handle.index >= ArraySize(r->material_slots))
		return NULL;

	const R_MaterialSlot *slot = &r->material_slots[handle.index];

	if (!slot->active || slot->generation != handle.generation)
		return NULL;

	return &slot->source;
}

static u64 R_MaterialRegistryBufferAddr(const R_MaterialRegistry *r)
{
	return G_DeviceBufferAddress(r->device, r->material_buffer);
}

static void R_MaterialRegistryBakeIntoGPU(const R_MaterialRegistry *r, const R_Material *material, R_GPU_Material *out)
{
	out->albedo_texture                       = R_MaterialRegistryResolveToBindless(r, material->albedo_texture);
	out->normal_texture                       = R_MaterialRegistryResolveToBindless(r, material->normal_texture);
	out->metallic_roughness_texture           = R_MaterialRegistryResolveToBindless(r, material->metallic_roughness_texture);
	out->emissive_texture                     = R_MaterialRegistryResolveToBindless(r, material->emissive_texture);
	out->occlusion_texture                    = R_MaterialRegistryResolveToBindless(r, material->occlusion_texture);
	
	out->albedo_factor                        = material->albedo_factor;
	out->normal_scale                         = material->normal_scale;
	out->metallic_factor                      = material->metallic_factor;
	out->roughness_factor                     = material->roughness_factor;
	out->emissive_factor                      = material->emissive_factor;
	out->emissive_intensity                   = material->emissive_intensity;
	out->occlusion_intensity                  = material->occlusion_intensity;

	out->ior                                  = material->ior;
	
	out->transmission_texture                 = R_MaterialRegistryResolveToBindless(r, material->transmission_texture);
	out->thickness_texture                    = R_MaterialRegistryResolveToBindless(r, material->thickness_texture);

	out->transmission_factor                  = material->transmission_factor;
	out->thickness_factor                     = material->thickness_factor;

	out->attenuation_colour                   = material->attenuation_colour;
	out->attenuation_distance                 = material->attenuation_distance;

	out->specular_texture                     = R_MaterialRegistryResolveToBindless(r, material->specular_texture);
	out->specular_colour_texture              = R_MaterialRegistryResolveToBindless(r, material->specular_colour_texture);

	out->specular_factor                      = material->specular_factor;
	out->specular_colour_factor               = material->specular_colour_factor;

	out->clearcoat_texture                    = R_MaterialRegistryResolveToBindless(r, material->clearcoat_texture);
	out->clearcoat_roughness_texture          = R_MaterialRegistryResolveToBindless(r, material->clearcoat_roughness_texture);

	out->clearcoat_factor                     = material->clearcoat_factor;
	out->clearcoat_roughness_factor           = material->clearcoat_roughness_factor;

	out->sheen_colour_texture                 = R_MaterialRegistryResolveToBindless(r, material->sheen_colour_texture);
	out->sheen_roughness_texture              = R_MaterialRegistryResolveToBindless(r, material->sheen_roughness_texture);

	out->sheen_colour_factor                  = material->sheen_colour_factor;
	out->sheen_roughness_factor               = material->sheen_roughness_factor;
	
	out->iridescence_texture                  = R_MaterialRegistryResolveToBindless(r, material->iridescence_texture);
	out->iridescence_thickness_texture        = R_MaterialRegistryResolveToBindless(r, material->iridescence_thickness_texture);

	out->iridescence_factor                   = material->iridescence_factor;
	out->iridescence_ior                      = material->iridescence_ior;
	out->iridescence_thickness_min_nanometers = material->iridescence_thickness_min_nanometers;
	out->iridescence_thickness_max_nanometers = material->iridescence_thickness_max_nanometers;

	out->double_sided                         = material->double_sided;
	out->unlit                                = material->unlit;
	out->alpha_cutoff                         = material->alpha_cutoff;
	out->alpha_mode                           = material->alpha_mode;
}

static b32 R_MaterialRegistryHandleIsValid(const R_MaterialRegistry *r, R_SceneHandle handle)
{
	if (handle.index >= ArraySize(r->material_slots))
		return false;

	const R_MaterialSlot *slot = &r->material_slots[handle.index];
	return slot->active && slot->generation == handle.generation;
}

static void R_MaterialRegistryFlushIfDirty(R_MaterialRegistry *r)
{
	if (!r->material_buffer_dirty)
		return;

	G_DeviceBufferWrite(r->device,
						  r->material_buffer,
						  r->material_gpus,
						  sizeof(r->material_gpus), 0);

	r->material_buffer_dirty = false;
}

static G_BindlessIndex R_MaterialRegistryResolveToBindless(const R_MaterialRegistry *r, G_TextureKey key)
{
	if (G_TextureKeyIsNull(key))
		return G_BINDLESS_INDEX_INVALID;

	G_TextureViewKey view_key = G_DeviceTextureViewAuto(r->device, key);
	return G_DeviceTextureViewBindless(r->device, view_key);
}
