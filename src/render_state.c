
internal u32 RenderStateUploadMesh(RenderState *context, Mesh *mesh)
{
	for (i32 i = 0; i < context->mesh_count; i++) {
		if (MeshesEqual(context->meshes[i].original, mesh))
			return i;
	}

	RenderMesh render_mesh = {0};
	{
		render_mesh.original = mesh;

		render_mesh.is_merged = false;

		render_mesh.first_vertex = 0;
		render_mesh.first_index = 0;
		render_mesh.index_count = mesh->index_count;
	}

	context->meshes[context->mesh_count] = render_mesh;

	return context->mesh_count++;
}

internal u32 RenderStateUploadMaterial(RenderState *context, Assets *assets,
					 Material *material)
{
	for (i32 i = 0; i < context->material_count; i++) {
		if (MaterialsEqual(&context->materials[i], material))
			return i;
	}

	context->materials[context->material_count] = *material;

	GPU_Material gpu_material = {0};

	gpu_material.diffuse_texture            = FetchStandardImageView(AssetsImageFromHandle(assets, material->diffuse_texture_handle))->resource_id;
	gpu_material.normal_texture             = FetchStandardImageView(AssetsImageFromHandle(assets, material->normal_texture_handle))->resource_id;
	gpu_material.emissive_texture           = FetchStandardImageView(AssetsImageFromHandle(assets, material->emissive_texture_handle))->resource_id;
	gpu_material.metallic_roughness_texture = FetchStandardImageView(AssetsImageFromHandle(assets, material->metallic_roughness_texture_handle))->resource_id;
	gpu_material.ambient_texture            = FetchStandardImageView(AssetsImageFromHandle(assets, material->ambient_texture_handle))->resource_id;

	GPUBufferWrite(&context->material_buffer, &gpu_material,
		       sizeof(GPU_Material),
		       sizeof(GPU_Material) * context->material_count);

	return context->material_count++;
}
