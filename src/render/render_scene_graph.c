
static void R_SceneGraphInit(R_SceneGraph *sg, LOG_Channel log_channel)
{
	sg->log_channel = log_channel;

	for (u32 i = 0; i < ArraySize(sg->object_slots); i++)
		sg->object_slots[i].generation = 1;
	
	for (u32 i = 0; i < ArraySize(sg->light_slots); i++)
		sg->light_slots[i].generation = 1;

	for (i32 i = ArraySize(sg->object_slots) - 1; i > 0; i--)
		sg->object_free_list[sg->object_free_count++] = i - 1;
	
	for (i32 i = ArraySize(sg->light_slots) - 1; i > 0; i--)
		sg->light_free_list[sg->light_free_count++] = i - 1;	
}

static void R_SceneGraphDestroy(R_SceneGraph *sg)
{
}

static R_SceneHandle R_SceneGraphObjectCreate(R_SceneGraph *sg, const R_ObjectDesc *desc)
{
	DebugLogAssert(sg->log_channel,
				   sg->object_free_count > 0,
				   "Ran out of free object slots.");

	sg->object_free_count--;
	u32 slot_index = sg->object_free_list[sg->object_free_count];
	R_ObjectSlot *slot = &sg->object_slots[slot_index];

	slot->transform = desc->transform;
	slot->normal_matrix = M4RemoveTranslation(M4Inverse(M4Transpose(desc->transform)));
	slot->sphere_bounds = desc->sphere_bounds;
	slot->mesh = desc->mesh;
	slot->material = desc->material;

	slot->skinning_palette = NULL;
	slot->skinning_joint_count = 0;

	slot->active = true;

	sg->object_count++;

	R_SceneHandle handle = {0};
	handle.index = slot_index;
	handle.generation = slot->generation;

	return handle;
}

static void R_SceneGraphObjectDestroy(R_SceneGraph *sg, R_SceneHandle handle)
{
	R_ObjectSlot *slot = R_SceneGraphObjectGetSlot(sg, handle);

	if (!slot)
		return;

	slot->active = false;
	slot->generation++;

	sg->object_free_list[sg->object_free_count++] = handle.index;
	sg->object_count--;
}

static u32 R_SceneGraphObjectCount(const R_SceneGraph *sg)
{
	return sg->object_count;
}

static R_ObjectSlot *R_SceneGraphObjectGetSlot(R_SceneGraph *sg, R_SceneHandle handle)
{
	if (handle.index >= ArraySize(sg->object_slots))
		return NULL;

	R_ObjectSlot *slot = &sg->object_slots[handle.index];

	if (!slot->active || slot->generation != handle.generation)
		return NULL;

	return slot;
}

static void R_SceneGraphObjectSetTransform(R_SceneGraph *sg, R_SceneHandle handle, m4 transform)
{
	R_ObjectSlot *slot = R_SceneGraphObjectGetSlot(sg, handle);

	if (!slot)
		return;
	
	slot->transform = transform;
	slot->normal_matrix = M4RemoveTranslation(M4Inverse(M4Transpose(slot->transform)));
}

static void R_SceneGraphObjectSetSphereBounds(R_SceneGraph *sg, R_SceneHandle handle, v4 sphere_bounds)
{
	R_ObjectSlot *slot = R_SceneGraphObjectGetSlot(sg, handle);

	if (!slot)
		return;
	
	slot->sphere_bounds = sphere_bounds;
}

static void R_SceneGraphObjectSetMaterial(R_SceneGraph *sg, R_SceneHandle handle, R_SceneHandle material)
{
	R_ObjectSlot *slot = R_SceneGraphObjectGetSlot(sg, handle);

	if (!slot)
		return;
	
	slot->material = material;
}

static void R_SceneGraphObjectSetMesh(R_SceneGraph *sg, R_SceneHandle handle, R_SceneHandle mesh)
{
	R_ObjectSlot *slot = R_SceneGraphObjectGetSlot(sg, handle);

	if (!slot)
		return;
	
	slot->mesh = mesh;
}

static void R_SceneGraphObjectSetSkinning(R_SceneGraph *sg, R_SceneHandle handle, const AN_Palette *palette)
{
	R_ObjectSlot *slot = R_SceneGraphObjectGetSlot(sg, handle);

	if (!slot)
		return;
	
	slot->skinning_palette = palette->matrices;
	slot->skinning_joint_count = palette->joint_count;
}

static b32 R_SceneGraphObjectHandleIsValid(const R_SceneGraph *sg, R_SceneHandle handle)
{
	if (handle.index >= ArraySize(sg->object_slots))
		return false;

	const R_ObjectSlot *slot = &sg->object_slots[handle.index];
	return slot->active && slot->generation == handle.generation;
}

static R_SceneHandle R_SceneGraphLightCreate(R_SceneGraph *sg, const R_Light *light)
{
	DebugLogAssert(sg->log_channel,
				   sg->light_free_count > 0,
				   "Ran out of free light slots.");

	sg->light_free_count--;
	u32 slot_index = sg->light_free_list[sg->light_free_count];
	R_LightSlot *slot = &sg->light_slots[slot_index];

	slot->light = *light;
	
	slot->active = true;

	sg->light_count++;

	R_SceneHandle handle = {0};
	handle.index = slot_index;
	handle.generation = slot->generation;

	return handle;
}

static void R_SceneGraphLightDestroy(R_SceneGraph *sg, R_SceneHandle handle)
{
	R_LightSlot *slot = R_SceneGraphLightGetSlot(sg, handle);

	if (!slot)
		return;

	slot->active = false;
	slot->generation++;

	sg->light_free_list[sg->light_free_count++] = handle.index;
	sg->light_count--;
}

static u32 R_SceneGraphLightCount(const R_SceneGraph *sg)
{
	return sg->light_count;
}

static R_LightSlot *R_SceneGraphLightGetSlot(R_SceneGraph *sg, R_SceneHandle handle)
{
	if (handle.index >= ArraySize(sg->light_slots))
		return NULL;

	R_LightSlot *slot = &sg->light_slots[handle.index];

	if (!slot->active || slot->generation != handle.generation)
		return NULL;

	return slot;
}

static void R_SceneGraphLightSetPosition(R_SceneGraph *sg, R_SceneHandle handle, v3 position)
{
	R_LightSlot *slot = R_SceneGraphLightGetSlot(sg, handle);

	if (!slot)
		return;

	slot->light.position = position;
}

static void R_SceneGraphLightSetColour(R_SceneGraph *sg, R_SceneHandle handle, v3 colour)
{
	R_LightSlot *slot = R_SceneGraphLightGetSlot(sg, handle);

	if (!slot)
		return;

	slot->light.colour = colour;
}

static void R_SceneGraphLightSetIntensity(R_SceneGraph *sg, R_SceneHandle handle, f32 intensity)
{
	R_LightSlot *slot = R_SceneGraphLightGetSlot(sg, handle);

	if (!slot)
		return;

	slot->light.intensity = intensity;
}

static b32 R_SceneGraphLightHandleIsValid(const R_SceneGraph *sg, R_SceneHandle handle)
{
	if (handle.index >= ArraySize(sg->light_slots))
		return false;

	const R_LightSlot *slot = &sg->light_slots[handle.index];
	return slot->active && slot->generation == handle.generation;
}
