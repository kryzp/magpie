
static void R_SceneInit(R_Scene *scene, Arena *arena, LOG_Channel log_channel)
{
	scene->log_channel = log_channel;

	R_SceneGraphInit(&scene->graph, osapi->LogChannelOpenFrom(log_channel, String8Lit("GRAPH")));
	R_MeshRegistryInit(&scene->meshes, arena, osapi->LogChannelOpenFrom(log_channel, String8Lit("MESH")));
	R_MaterialRegistryInit(&scene->materials, osapi->LogChannelOpenFrom(log_channel, String8Lit("MATERIAL")));

	DebugLogI(scene->log_channel, "Initialized.");
}

static void R_SceneDestroy(R_Scene *scene)
{
	R_MaterialRegistryDestroy(&scene->materials);
	R_MeshRegistryDestroy(&scene->meshes);
	R_SceneGraphDestroy(&scene->graph);
	
	DebugLogI(scene->log_channel, "Destroyed.");
}

static R_ModelImportReceipt R_SceneImportModel(R_Scene *scene, G_CmdBuffer *cmd, Arena *arena, A_Handle asset_handle)
{
	A_ModelData *model_asset = &A_GetNow(asset_handle)->model;
	
	u32 sub_model_count = model_asset->sub_model_count;
	const A_SubModel *sub_models = model_asset->sub_models;
	
	u32 actual_count = sub_model_count;
	
	/*
	if (sub_model_count > max_count)
	{
		DebugLogW(scene->log_channel,
				  "Hit max entry count! Truncating total sub model count %u down to %u.",
				  sub_model_count, max_count);

		actual_count = max_count;
	}
	*/
	
	R_ModelImportReceipt receipt = {0};
	receipt.count = actual_count;
	receipt.entries = ArenaPushArray(arena, R_ModelImportEntry, actual_count);

	for (u32 i = 0; i < actual_count; i++)
	{
		const A_SubModel *sub = &sub_models[i];
		R_ModelImportEntry *entry = &receipt.entries[i];

		R_MeshDesc mesh_desc = {0};
		mesh_desc.vertex_buffer = sub->vertex_buffer;
		mesh_desc.index_buffer = sub->index_buffer;
		mesh_desc.vertex_count = sub->vertex_count;
		mesh_desc.index_count = sub->index_count;
		mesh_desc.skin_buffer = sub->skin_buffer;

		v3 centre = V3MulF32(V3Add(sub->bounds_min, sub->bounds_max), 0.5f);
		f32 radius = V3Length(V3Sub(sub->bounds_max, centre));

		entry->mesh = R_MeshRegistryCreateMesh(&scene->meshes, cmd, &mesh_desc);
		entry->material = R_MaterialRegistryAddFromAssets(&scene->materials, &sub->material);
		entry->transform = sub->transform;
		entry->sphere_bounds = v4(centre.x, centre.y, centre.z, radius);
		entry->skin_index = sub->skin_index;
	}

	return receipt;
}

static R_Model R_SceneModelCreate(R_Scene *scene, G_CmdBuffer *cmd, Arena *arena, A_Handle asset_handle, b32 animated)
{
	Arena *conflicts[] = { arena };

	ScratchArena scratch = ScratchBegin(conflicts, 1);

	R_ModelImportReceipt receipt = R_SceneImportModel(scene, cmd, scratch.arena, asset_handle);

	R_Model model = {0};
	model.asset_handle = asset_handle;
	model.animator_handle = AN_HandleNull();
	model.object_count = receipt.count;
	model.objects = ArenaPushArray(arena, R_SceneHandle, model.object_count);
	model.local_transforms = ArenaPushArray(arena, m4, model.object_count);
	model.skin_indices = ArenaPushArray(arena, m4, model.object_count);

	for (u32 i = 0; i < model.object_count; i++)
	{
		R_ModelImportEntry *entry = &receipt.entries[i];

		R_ObjectDesc desc = {0};
		desc.transform = entry->transform;
		desc.sphere_bounds = entry->sphere_bounds;
		desc.mesh = entry->mesh;
		desc.material = entry->material;

		model.objects[i] = R_SceneGraphObjectCreate(&scene->graph, &desc);
		model.local_transforms[i] = entry->transform;
		model.skin_indices[i] = entry->skin_index;
	}

	if (animated)
		model.animator_handle = AN_SystemCreateInstance(asset_handle);

	ScratchRelease(&scratch);

	return model;
}

static void R_SceneModelDestroy(R_Scene *scene, R_Model *model)
{
	for (u32 i = 0; i < model->object_count; i++)
		R_SceneGraphObjectDestroy(&scene->graph, model->objects[i]);

	if (!AN_HandleIsNull(model->animator_handle))
		AN_SystemKillInstance(model->animator_handle);
}

static void R_SceneModelSetRootTransform(R_Scene *scene, R_Model *model, m4 root_transform)
{
	for (u32 i = 0; i < model->object_count; i++)
	{
		m4 world_matrix = M4MulM4(root_transform, model->local_transforms[i]);
		R_SceneGraphObjectSetTransform(&scene->graph, model->objects[i], world_matrix);
	}
}

static void R_SceneModelUpdateSkinning(R_Scene *scene, R_Model *model)
{
	if (AN_HandleIsNull(model->animator_handle))
		return;

	for (u32 i = 0; i < model->object_count; i++)
	{
		i32 skin_index = model->skin_indices[i];

		if (skin_index < 0)
			continue;

		AN_Palette palette = AN_GetPalette(model->animator_handle, skin_index);

		R_SceneGraphObjectSetSkinning(&scene->graph, model->objects[i], &palette);
	}
}
