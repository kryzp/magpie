
static void R_SceneInit(R_Scene *scene, Arena *arena, G_Device *device, A_Assets *assets, LOG_Channel log_channel)
{
	scene->assets = assets;
	scene->log_channel = log_channel;

	R_SceneGraphInit(&scene->graph, osapi->LogChannelOpenFrom(log_channel, String8Lit("GRAPH")));
	R_MeshRegistryInit(&scene->meshes, arena, device, osapi->LogChannelOpenFrom(log_channel, String8Lit("MESH")));
	R_MaterialRegistryInit(&scene->materials, device, assets, osapi->LogChannelOpenFrom(log_channel, String8Lit("MATERIAL")));

	DebugLogI(scene->log_channel, "Initialized.");
}

static void R_SceneDestroy(R_Scene *scene)
{
	R_MaterialRegistryDestroy(&scene->materials);
	R_MeshRegistryDestroy(&scene->meshes);
	R_SceneGraphDestroy(&scene->graph);
	
	DebugLogI(scene->log_channel, "Destroyed.");
}

static void R_SceneDrawIndirect(const R_Scene *scene,
					G_CmdBuffer *cmd,
					G_BufferKey indirect_buffer,
					G_BufferKey count_buffer)
{
	for (u64 i = 0; i < scene->meshes.geometry_page_count; i++)
	{
		const R_GeometryPage *page = &scene->meshes.geometry_pages[i];

		u64 max_draws_per_page = R_SCENE_GRAPH_MAX_OBJECTS;

		u64 indirect_offset = i * sizeof(R_GPU_IndirectDraw) * max_draws_per_page;
		u64 count_offset = i * sizeof(u32);
		
		G_CmdBindIndexBuffer(cmd,
							 page->index_buffer,
							 0, VK_WHOLE_SIZE,
							 VK_INDEX_TYPE_UINT32);
		
		G_CmdDrawIndexedIndirectCount(cmd,
									  indirect_buffer, indirect_offset,
									  count_buffer, count_offset,
									  max_draws_per_page,
									  sizeof(R_GPU_IndirectDraw));
	}
}

static R_ModelImportReceipt R_SceneImportModel(R_Scene *scene, G_CmdBuffer *cmd, Arena *arena, A_Handle handle, u32 max_count)
{
	A_ModelData *model_asset = &A_GetNow(scene->assets, handle)->model;
	
	u32 sub_model_count = model_asset->sub_model_count;
	const A_SubModel *sub_models = model_asset->sub_models;
	
	u32 actual_count = sub_model_count;
	
	if (sub_model_count > max_count)
	{
		DebugLogW(scene->log_channel,
				  "Hit max entry count! Truncating total sub model count %u down to %u.",
				  sub_model_count, max_count);

		actual_count = max_count;
	}
	
	R_ModelImportReceipt receipt = {0};
	receipt.count = sub_model_count;
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
