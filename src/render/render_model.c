
internal R_Model R_ModelFromAsset(Arena *arena, R_Scene *scene, A_Handle asset_handle)
{
	const A_ModelAsset *model_asset = &A_GetOrBreak(asset_handle)->model;
	
	R_Model model = {0};
	model.asset_handle = asset_handle;

	for (u32 i = 0; i < model_asset->sub_model_count; i++)
	{
		const A_SubModel *asset_src = &model_asset->sub_models[i];

		R_SubModel *r_submodel = ArenaPushArray(arena, R_SubModel, 1);
		r_submodel->next = model.submodel_first;
		model.submodel_first = r_submodel;

		R_MeshDesc mesh_desc = {0};
		
		mesh_desc.vertices      = asset_src->vertices;
		mesh_desc.vertex_count  = asset_src->vertex_count;
		mesh_desc.vertex_stride = asset_src->vertex_stride;
		mesh_desc.indices       = asset_src->indices;
		mesh_desc.index_count   = asset_src->index_count;
		mesh_desc.index_type    = asset_src->index_type;
		mesh_desc.skin_buffer   = asset_src->skin_buffer;

		r_submodel->mesh = R_SceneAllocMesh(scene, &mesh_desc);
		
		r_submodel->material = R_SceneAddMaterialFromAssets(scene, &asset_src->material);
		r_submodel->local_transform = asset_src->transform;
		r_submodel->skin_index = asset_src->skin_index;

		v3 bounds_min = asset_src->bounds_min;
		v3 bounds_max = asset_src->bounds_max;
		v3 local_centre = V3MulF32(V3Add(bounds_min, bounds_max), 0.5f);
		v3 dx = V3Sub(bounds_max, bounds_min);
		f32 local_radius = V3Length(dx) * 0.5f;

		r_submodel->local_sphere_bounds = v4(local_centre.x,
											 local_centre.y,
											 local_centre.z,
											 local_radius);
	}

	return model;
}

internal R_ModelCatalogueEntry *R_ModelCatalogueTryFindEntry(R_ModelCatalogue *catalogue, A_Handle asset_handle)
{
	for (R_ModelCatalogueEntry *entry = catalogue->entry_first_sentinel.next; 
		 entry != &catalogue->entry_first_sentinel; 
		 entry = entry->next)
	{
		if (A_HandleMatch(entry->asset_handle, asset_handle))
			return entry;
	}

	return NULL;
}

internal void R_ModelCatalogueInit(R_ModelCatalogue *catalogue, Arena *arena)
{
	catalogue->arena = arena;

	catalogue->entry_first_sentinel.next = &catalogue->entry_first_sentinel;
	catalogue->entry_first_sentinel.prev = &catalogue->entry_first_sentinel;

	catalogue->first_free_entry_sentinel.next = &catalogue->first_free_entry_sentinel;
	catalogue->first_free_entry_sentinel.prev = &catalogue->first_free_entry_sentinel;
}

internal void R_ModelCatalogueEquipScene(R_ModelCatalogue *catalogue, R_Scene *scene)
{
	catalogue->equipped_scene = scene;
}

internal void R_ModelCatalogueDestroy(R_ModelCatalogue *catalogue)
{
	for (R_ModelCatalogueEntry *entry = catalogue->entry_first_sentinel.next; 
		 entry != &catalogue->entry_first_sentinel; 
		 entry = entry->next)
	{
		for (R_SubModel *submodel = entry->model.submodel_first; submodel; submodel = submodel->next)
		{
			R_SceneFreeMesh(catalogue->equipped_scene, submodel->mesh);
			R_SceneFreeMaterial(catalogue->equipped_scene, submodel->material);
		}
	}
}

internal R_Model *R_ModelCatalogueCreateModel(R_ModelCatalogue *catalogue, A_Handle asset_handle)
{
	R_ModelCatalogueEntry *existing = R_ModelCatalogueTryFindEntry(catalogue, asset_handle);

	if (existing)
	{
		existing->ref_count++;
		return &existing->model;
	}

	R_ModelCatalogueEntry *entry = ArenaPushArray(catalogue->arena, R_ModelCatalogueEntry, 1);
	entry->asset_handle = asset_handle;
	entry->model = R_ModelFromAsset(catalogue->arena, catalogue->equipped_scene, asset_handle);
	entry->ref_count = 1;
	
	entry->next = catalogue->entry_first_sentinel.next;
	entry->prev = &catalogue->entry_first_sentinel;

	entry->next->prev = entry;
	entry->prev->next = entry;
	
	return &entry->model;
}

internal void R_ModelCatalogueReleaseModel(R_ModelCatalogue *catalogue, A_Handle asset_handle)
{
	R_ModelCatalogueEntry *entry = R_ModelCatalogueTryFindEntry(catalogue, asset_handle);

	if (!entry)
		return;

	if (entry->ref_count > 0)
		entry->ref_count--;

	if (entry->ref_count == 0)
	{
		for (R_SubModel *submodel = entry->model.submodel_first; submodel; submodel = submodel->next)
		{
			R_SceneFreeMesh(catalogue->equipped_scene, submodel->mesh);
			R_SceneFreeMaterial(catalogue->equipped_scene, submodel->material);
		}

		entry->prev->next = entry->next;
		entry->next->prev = entry->prev;

		entry->next = catalogue->first_free_entry_sentinel.next;
		entry->prev = &catalogue->first_free_entry_sentinel;

		entry->next->prev = entry;
		entry->prev->next = entry;
	}
}

internal R_Model *R_ModelCatalogueTryFindModel(R_ModelCatalogue *catalogue, A_Handle asset_handle)
{
	R_ModelCatalogueEntry *entry = R_ModelCatalogueTryFindEntry(catalogue, asset_handle);

	if (!entry)
		return NULL;

	return &entry->model;
}

internal R_ModelInstance R_ModelInstanceCreate(R_ModelCatalogue *catalogue, A_Handle asset_handle, m4 initial_transform)
{
	R_Model *model = R_ModelCatalogueCreateModel(catalogue, asset_handle);

	R_ModelInstance instance = {0};
	instance.asset_handle = asset_handle;
	instance.root_transform = initial_transform;
	instance.animator_handle = AN_HandleNull();

	b32 needs_animator = false;

	for (R_SubModel *submodel = model->submodel_first; submodel; submodel = submodel->next)
	{
		if (submodel->skin_index >= 0)
		{
			needs_animator = true;
			break;
		}
	}

	if (needs_animator)
	{
		instance.has_animator = true;
		instance.animator_handle = AN_SystemCreateInstance(asset_handle);
	}

	R_SubModelInstance *i_submodel_tail = NULL;
 
	for (R_SubModel *submodel = model->submodel_first; submodel; submodel = submodel->next)
	{
		R_SubModelInstance *i_submodel = ArenaPushArray(catalogue->arena, R_SubModelInstance, 1);
		i_submodel->next = NULL;
		
		if (i_submodel_tail)
			i_submodel_tail->next = i_submodel;
		else
			instance.submodel_first = i_submodel;
		
		i_submodel_tail = i_submodel;
		
		i_submodel->handle = R_SceneEntityCreate(catalogue->equipped_scene, R_EntityType_Object);
		
		R_SceneSetObjectMesh              (catalogue->equipped_scene, i_submodel->handle, submodel->mesh);
		R_SceneSetObjectMaterial          (catalogue->equipped_scene, i_submodel->handle, submodel->material);
		R_SceneSetObjectLocalSphereBounds (catalogue->equipped_scene, i_submodel->handle, submodel->local_sphere_bounds);
		
		if (instance.has_animator)
		{
			AN_Palette palette = AN_GetPalette(instance.animator_handle, submodel->skin_index);
			R_SceneSetObjectSkinning(catalogue->equipped_scene, i_submodel->handle, palette.matrices, palette.joint_count);
		}
	}

	R_ModelInstanceSetTransform(catalogue, &instance, initial_transform);

	return instance;
}

internal R_ModelInstance R_ModelInstanceCreateFromPath(R_ModelCatalogue *catalogue, String8 asset_path, m4 initial_transform)
{
	A_Handle asset_handle = A_RequireAssetBlocking(catalogue->arena, asset_path);
	AssertTrue(asset_handle.type == A_Type_Model && "Attempting to load model asset and it's not a model!!!");
	return R_ModelInstanceCreate(catalogue, asset_handle, initial_transform);
}

internal void R_ModelInstanceDestroy(R_ModelCatalogue *catalogue, R_ModelInstance *instance)
{
	for (R_SubModelInstance *submodel = instance->submodel_first; submodel; submodel = submodel->next)
		R_SceneEntityDestroy(catalogue->equipped_scene, submodel->handle);

	if (instance->has_animator)
		AN_SystemKillInstance(instance->animator_handle);

	R_ModelCatalogueReleaseModel(catalogue, instance->asset_handle);
}

internal void R_ModelInstanceSetTransform(R_ModelCatalogue *catalogue, R_ModelInstance *instance, m4 root_transform)
{
	R_Model *original = R_ModelCatalogueTryFindModel(catalogue, instance->asset_handle);
	
	instance->root_transform = root_transform;

	R_SubModel *original_submodel = original->submodel_first;
	R_SubModelInstance *instance_submodel = instance->submodel_first;
	
	while (original_submodel && instance_submodel)
	{
		m4 world_matrix = M4MulM4(instance->root_transform, original_submodel->local_transform);

		R_SceneSetObjectTransform(catalogue->equipped_scene, instance_submodel->handle, world_matrix);

		original_submodel = original_submodel->next;
		instance_submodel = instance_submodel->next;
	}
}
