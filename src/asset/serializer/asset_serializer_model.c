
/*
 * magpie is right-handed z-up.
 * glTF is right-handed y-up.
 * therefore we gotta translate between 'em.
 */

const global m4 ast_gltf_basis = {
	1.f,  0.f,  0.f,  0.f, // c0
	0.f,  0.f,  1.f,  0.f, // c1
	0.f, -1.f,  0.f,  0.f, // c2
	0.f,  0.f,  0.f,  1.f  // c3
};

const global m4 ast_gltf_basis_inv = {
	1.f,  0.f,  0.f,  0.f, // c0
	0.f,  0.f, -1.f,  0.f, // c1
	0.f,  1.f,  0.f,  0.f, // c2
	0.f,  0.f,  0.f,  1.f  // c3
};

internal v3
AST_GltfTransformV3(v3 v)
{
	return v3(v.x, -v.z, v.y);
}

internal m4
AST_GltfFloat16ToM4(const cgltf_float m[16])
{
	return (m4) {
		m[0],  m[1],  m[2],  m[3],
		m[4],  m[5],  m[6],  m[7],
		m[8],  m[9],  m[10], m[11],
		m[12], m[13], m[14], m[15]
	};
}

internal m4
AST_GltfTransformM4(m4 m)
{
	// magpie = basis * cgltf * basis^-1
	return M4MulM4(ast_gltf_basis, M4MulM4(m, ast_gltf_basis_inv));
}

typedef struct AST_ModelLoadMesh AST_ModelLoadMesh;
struct AST_ModelLoadMesh
{
	AST_ModelLoadMesh *next;

	m4 transform;

	v3 bounds_min;
	v3 bounds_max;

	u32 vertex_count;
	AST_ModelVertex *vertices;

	u32 index_count;
	AST_ModelIndex *indices;

	AST_ModelMaterial material;
};

typedef struct AST_ModelLoadDep AST_ModelLoadDep;
struct AST_ModelLoadDep
{
	AST_ModelLoadDep *next;
	AST_Handle handle;
};

typedef struct AST_ModelLoadData AST_ModelLoadData;
struct AST_ModelLoadData
{
	AST_ModelLoadMesh *first_mesh;
	u32 mesh_count;

	AST_ModelLoadDep *first_dep;
	u32 dep_count;

	u64 total_vertex_bytes;
	u64 total_index_bytes;
};

internal void
AST_ModelAddDependency(AST_ModelLoadData *load, Arena *arena, AST_Handle handle)
{
	AST_ModelLoadDep *dep = ArenaPushArray(arena, AST_ModelLoadDep, 1);
	dep->handle = handle;
	dep->next = load->first_dep;
	load->first_dep = dep;
	load->dep_count++;
}

internal AST_Handle
AST_ModelTryFetchTexture(const AST_Context *ctx,
						 AST_ModelLoadData *load,
						 String8 directory,
						 const cgltf_texture_view *view)
{
	if (!view->texture || !view->texture->image)
		return AST_HandleNull();

	const cgltf_image *image = view->texture->image;
	
	// TODO: handle data: URIs (base64 embedded)
	//       and bufferview-backed images (.glb).
	if (!image->uri)
	{
		DebugLogB(ctx->log_channel, "URIs not supported yet.");
		return AST_HandleNull();
	}

	if (strncmp(image->uri, "data:", 5) == 0)
		return AST_HandleNull();

	String8 relative = String8Init(image->uri, strlen(image->uri));
	String8 full_path = String8Append(ctx->scope, directory, relative);

	AST_Handle handle = AST_FromFilePath(ctx->assets, full_path);

	if (AST_IsValid(ctx->assets, handle))
		AST_ModelAddDependency(load, ctx->scope, handle);

	return handle;
}

internal AST_ModelMaterial
AST_ModelResolveMaterial(const AST_Context *ctx,
						 AST_ModelLoadData *load,
						 String8 directory,
						 const cgltf_material *gltf_mat)
{
	AST_ModelMaterial mat = {0};

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

	mat.alpha_mode                           = AST_AlphaMode_Opaque;
	mat.alpha_cutoff                         = 0.5f;
	mat.double_sided                         = false;
	mat.unlit                                = false;

	mat.reflection_mode                      = AST_ReflectionMode_Default;
	mat.reflection_plane                     = v4(0.f, 0.f, 0.f, 0.f);


	// STANDARD METALLIC-ROUGHNESS OPAQUE PBR.
	if (gltf_mat->has_pbr_metallic_roughness)
	{
		const cgltf_pbr_metallic_roughness *pbr = &gltf_mat->pbr_metallic_roughness;

		mat.albedo             = AST_ModelTryFetchTexture(ctx, load, directory, &pbr->base_color_texture);
		mat.metallic_roughness = AST_ModelTryFetchTexture(ctx, load, directory, &pbr->metallic_roughness_texture);

		mat.albedo_factor      = v4(pbr->base_color_factor[0],
									pbr->base_color_factor[1],
									pbr->base_color_factor[2],
									pbr->base_color_factor[3]);
		
		mat.metallic_factor    = pbr->metallic_factor;
		mat.roughness_factor   = pbr->roughness_factor;
	}


	// NORMALS.
	mat.normal       = AST_ModelTryFetchTexture(ctx, load, directory, &gltf_mat->normal_texture);
	mat.normal_scale = gltf_mat->normal_texture.scale;


	// OCCLUSION
	mat.occlusion           = AST_ModelTryFetchTexture(ctx, load, directory, &gltf_mat->occlusion_texture);
	mat.occlusion_intensity = gltf_mat->occlusion_texture.scale;


	// EMISSIVE
	mat.emissive        = AST_ModelTryFetchTexture(ctx, load, directory, &gltf_mat->emissive_texture);
	mat.emissive_factor = v3(gltf_mat->emissive_factor[0],
							 gltf_mat->emissive_factor[1],
							 gltf_mat->emissive_factor[2]);


	// EMISSIVE STRENGTH.
	if (gltf_mat->has_emissive_strength)
		mat.emissive_intensity = gltf_mat->emissive_strength.emissive_strength;


	// INDEX OF REFRACTION.
	if (gltf_mat->has_ior)
		mat.ior = gltf_mat->ior.ior;


	// TRANSMISSION.
	if (gltf_mat->has_transmission)
	{
		mat.transmission        = AST_ModelTryFetchTexture(ctx, load, directory, &gltf_mat->transmission.transmission_texture);
		mat.transmission_factor = gltf_mat->transmission.transmission_factor;
	}


	// VOLUME.
	if (gltf_mat->has_volume)
	{
		mat.thickness            = AST_ModelTryFetchTexture(ctx, load, directory, &gltf_mat->volume.thickness_texture);
		mat.thickness_factor     = gltf_mat->volume.thickness_factor;

		mat.attenuation_colour   = v3(gltf_mat->volume.attenuation_color[0],
									  gltf_mat->volume.attenuation_color[1],
									  gltf_mat->volume.attenuation_color[2]);

		mat.attenuation_distance = gltf_mat->volume.attenuation_distance;
	}


	// SPECULAR.
	if (gltf_mat->has_specular)
	{
		mat.specular               = AST_ModelTryFetchTexture(ctx, load, directory, &gltf_mat->specular.specular_texture);
		mat.specular_colour        = AST_ModelTryFetchTexture(ctx, load, directory, &gltf_mat->specular.specular_color_texture);
		mat.specular_factor        = gltf_mat->specular.specular_factor;

		mat.specular_colour_factor = v3(gltf_mat->specular.specular_color_factor[0],
										gltf_mat->specular.specular_color_factor[1],
										gltf_mat->specular.specular_color_factor[2]);
	}


	// CLEARCOAT.
	if (gltf_mat->has_clearcoat)
	{
		mat.clearcoat                  = AST_ModelTryFetchTexture(ctx, load, directory, &gltf_mat->clearcoat.clearcoat_texture);
		mat.clearcoat_roughness        = AST_ModelTryFetchTexture(ctx, load, directory, &gltf_mat->clearcoat.clearcoat_roughness_texture);

		mat.clearcoat_factor           = gltf_mat->clearcoat.clearcoat_factor;
		mat.clearcoat_roughness_factor = gltf_mat->clearcoat.clearcoat_roughness_factor;
	}


	// SHEEN.
	if (gltf_mat->has_sheen)
	{
		mat.sheen_colour           = AST_ModelTryFetchTexture(ctx, load, directory, &gltf_mat->sheen.sheen_color_texture);
		mat.sheen_roughness        = AST_ModelTryFetchTexture(ctx, load, directory, &gltf_mat->sheen.sheen_roughness_texture);

		mat.sheen_colour_factor    = v3(gltf_mat->sheen.sheen_color_factor[0],
										gltf_mat->sheen.sheen_color_factor[1],
										gltf_mat->sheen.sheen_color_factor[2]);

		mat.sheen_roughness_factor = gltf_mat->sheen.sheen_roughness_factor;
	}


	// IRIDESCENCE.
	if (gltf_mat->has_iridescence)
	{
		mat.iridescence                          = AST_ModelTryFetchTexture(ctx, load, directory, &gltf_mat->iridescence.iridescence_texture);
		mat.iridescence_thickness                = AST_ModelTryFetchTexture(ctx, load, directory, &gltf_mat->iridescence.iridescence_thickness_texture);

		mat.iridescence_factor                   = gltf_mat->iridescence.iridescence_factor;
		mat.iridescence_ior                      = gltf_mat->iridescence.iridescence_ior;

		mat.iridescence_thickness_min_nanometers = gltf_mat->iridescence.iridescence_thickness_min;
		mat.iridescence_thickness_max_nanometers = gltf_mat->iridescence.iridescence_thickness_max;
	}


	// DOUBLE SIDED.
	mat.double_sided = !!gltf_mat->double_sided;


	// UNLIT.
	mat.unlit = !!gltf_mat->unlit;


	// ALPHA.
	switch (gltf_mat->alpha_mode)
	{
		case cgltf_alpha_mode_opaque: mat.alpha_mode = AST_AlphaMode_Opaque; break;
		case cgltf_alpha_mode_mask:   mat.alpha_mode = AST_AlphaMode_Mask;   break;
		case cgltf_alpha_mode_blend:  mat.alpha_mode = AST_AlphaMode_Blend;  break;
		default:                      mat.alpha_mode = AST_AlphaMode_Opaque; break;
	}

	mat.alpha_cutoff = gltf_mat->alpha_cutoff;

	return mat;
}

internal void
AST_ModelProcessPrimitive(const AST_Context *ctx,
						  AST_ModelLoadData *load,
						  String8 directory,
						  const cgltf_primitive *prim,
						  m4 world_transform)
{
	Arena *arena = ctx->scope;

	
	// TODO: support more topologies
	DebugLogAssert(ctx->log_channel,
				   prim->type == cgltf_primitive_type_triangles,
				   "Only support triangle-primitive models.");


	// Locate attributes.

	const cgltf_accessor *positions = NULL;
	const cgltf_accessor *normals   = NULL;
	const cgltf_accessor *tangents  = NULL;
	const cgltf_accessor *texcoords = NULL;
	const cgltf_accessor *colours   = NULL;

	for (cgltf_size i = 0; i < prim->attributes_count; i++)
	{
		const cgltf_attribute *attr = &prim->attributes[i];

		switch (attr->type)
		{
			case cgltf_attribute_type_position: positions = attr->data; break;
			case cgltf_attribute_type_normal:   normals   = attr->data; break;
			case cgltf_attribute_type_tangent:  tangents  = attr->data; break;

			case cgltf_attribute_type_texcoord:
				if (attr->index == 0) texcoords = attr->data;
				break;

			case cgltf_attribute_type_color:
				if (attr->index == 0) colours = attr->data;
				break;

			default: break;
		}
	}

	if (!positions)
		return;


	// Vertices.

	u32 vert_count = (u32)positions->count;
	AST_ModelVertex *vertices = ArenaPushArray(arena, AST_ModelVertex, vert_count);

	v3 bmin = v3( MATH_MAX_F32,  MATH_MAX_F32,  MATH_MAX_F32);
	v3 bmax = v3(-MATH_MAX_F32, -MATH_MAX_F32, -MATH_MAX_F32);

	for (u32 i = 0; i < vert_count; i++)
	{
		AST_ModelVertex *v = &vertices[i];

		f32 pos[3] = {0};
		cgltf_accessor_read_float(positions, i, pos, 3);
		v->position = AST_GltfTransformV3(v3(pos[0], pos[1], pos[2]));

		bmin = V3MinOf(bmin, v->position);
		bmax = V3MaxOf(bmax, v->position);

		if (texcoords)
		{
			f32 uv[2] = {0};
			cgltf_accessor_read_float(texcoords, i, uv, 2);
			v->texcoord = v2(uv[0], uv[1]);
		}
		else
		{
			v->texcoord = v2x(0.f);
		}

		if (colours)
		{
			f32 col[4] = {1.f, 1.f, 1.f, 1.f};
			cgltf_size comps = cgltf_num_components(colours->type);
			if (comps > 4) comps = 4;
			cgltf_accessor_read_float(colours, i, col, comps);
			v->colour = v3(col[0], col[1], col[2]);
		}
		else
		{
			v->colour = v3x(1.f);
		}

		if (normals)
		{
			f32 n[3] = {0};
			cgltf_accessor_read_float(normals, i, n, 3);
			v->normal = AST_GltfTransformV3(v3(n[0], n[1], n[2]));
		}
		else
		{
			// TODO: derive smooth normals if missing.
			//       --> shouldn't be a problem as glTF assets usually have them... i think
			v->normal = v3x(0.f);
		}

		if (tangents)
		{
			// glTF tangents are vec4: xyz = tangent, w = bitangent sign.
			f32 t[4] = {0};
			cgltf_accessor_read_float(tangents, i, t, 4);

			v3 N = v->normal;
			v3 T = AST_GltfTransformV3(v3(t[0], t[1], t[2]));
			f32 sign = t[3];

			v->tangent   = T;
			v->bitangent = v3((N.y * T.z - N.z * T.y) * sign,
							  (N.z * T.x - N.x * T.z) * sign,
							  (N.x * T.y - N.y * T.x) * sign);
		}
		else
		{
			// TODO: integrate using something called ""mikktspace" for tangents.
			v->tangent   = v3x(0.f);
			v->bitangent = v3x(0.f);
		}
	}


	// Indices.

	u32 idx_count = 0;
	AST_ModelIndex *indices = NULL;

	if (prim->indices)
	{
		idx_count = (u32)prim->indices->count;
		indices = ArenaPushArray(arena, AST_ModelIndex, idx_count);

		for (u32 i = 0; i < idx_count; i++)
		{
			cgltf_uint idx = 0;
			cgltf_accessor_read_uint(prim->indices, i, &idx, 1);
			indices[i] = (AST_ModelIndex)idx;
		}
	}
	else
	{
		// Non-indexed primitive: emit sequential indices.
		idx_count = vert_count;
		indices = ArenaPushArray(arena, AST_ModelIndex, idx_count);

		for (u32 i = 0; i < idx_count; i++)
			indices[i] = (AST_ModelIndex)i;
	}


	// Material.

	AST_ModelMaterial material = {0};

	if (prim->material)
	{
		material = AST_ModelResolveMaterial(ctx, load, directory, prim->material);
	}
	else
	{
		// Default material per glTF spec.
		material.albedo_factor       = v4(1.f, 1.f, 1.f, 1.f);
		material.normal_scale        = 1.f;
		material.metallic_factor     = 1.f;
		material.roughness_factor    = 1.f;
		material.emissive_factor     = v3x(0.f);
		material.emissive_intensity  = 1.f;
		material.occlusion_intensity = 1.f;
		material.ior                 = 1.5f;
		material.alpha_mode          = AST_AlphaMode_Opaque;
		material.alpha_cutoff        = 0.5f;
	}


	// Push onto list.

	AST_ModelLoadMesh *mesh = ArenaPushArray(arena, AST_ModelLoadMesh, 1);
	mesh->next = load->first_mesh;
	load->first_mesh = mesh;
	load->mesh_count++;

	mesh->transform  = world_transform;
	mesh->bounds_min = bmin;
	mesh->bounds_max = bmax;

	mesh->vertex_count = vert_count;
	mesh->vertices     = vertices;
	mesh->index_count  = idx_count;
	mesh->indices      = indices;
	mesh->material     = material;

	load->total_vertex_bytes += vert_count * sizeof(AST_ModelVertex);
	load->total_index_bytes  += idx_count  * sizeof(AST_ModelIndex);
}

internal void
AST_ModelProcessNode(const AST_Context *ctx,
					 AST_ModelLoadData *load,
					 String8 directory,
					 const cgltf_node *node)
{
	if (node->mesh)
	{
		// cgltf_node_transform_world walks the parent chain for us.
		// Conjugate the resulting matrix into Z-up basis so that vertex positions
		// (which we rebase per-vertex) and the mesh transform agree.
		cgltf_float world[16];
		cgltf_node_transform_world(node, world);
		m4 world_transform = AST_GltfTransformM4(AST_GltfFloat16ToM4(world));

		for (cgltf_size i = 0; i < node->mesh->primitives_count; i++)
		{
			const cgltf_primitive *prim = &node->mesh->primitives[i];
			AST_ModelProcessPrimitive(ctx, load, directory, prim, world_transform);
		}
	}

	for (cgltf_size i = 0; i < node->children_count; i++)
		AST_ModelProcessNode(ctx, load, directory, node->children[i]);
}

internal AST_SerializerPipelineData
AST_ModelSerializerCpu(const AST_Context *ctx)
{
	ScratchArena scratch = ScratchBegin(&ctx->scope, 1);

	String8 file_path = AST_ContextSystemFilePath(ctx, scratch.arena);

	AST_ModelLoadData *load = ArenaPushArray(ctx->scope, AST_ModelLoadData, 1);
	MemZeroStruct(load);

	AST_SerializerPipelineData result = {0};
	result.data = load;

	cgltf_options options = {0};
	cgltf_data *gltf = NULL;

	if (cgltf_parse_file(&options, (const char *)file_path.str, &gltf) != cgltf_result_success)
	{
		result.failed = true;
		goto end;
	}

	if (cgltf_load_buffers(&options, gltf, (const char *)file_path.str) != cgltf_result_success)
	{
		result.failed = true;
		goto end;
	}

	if (cgltf_validate(gltf) != cgltf_result_success)
	{
		result.failed = true;
		goto end;
	}

	String8 directory = IO_PathGetFileDirectory(scratch.arena, ctx->metadata.path);

	const cgltf_scene *scene = gltf->scene;
	if (!scene && gltf->scenes_count > 0)
		scene = &gltf->scenes[0];

	if (!scene)
	{
		result.failed = true;
		goto end;
	}

	for (cgltf_size i = 0; i < scene->nodes_count; i++)
		AST_ModelProcessNode(ctx, load, directory, scene->nodes[i]);

	result.stage_size = load->total_vertex_bytes + load->total_index_bytes;
	result.failed = false;

	result.dependency_count = load->dep_count;

	if (load->dep_count > 0)
	{
		result.dependencies = ArenaPushArray(ctx->scope, AST_Handle, load->dep_count);

		AST_ModelLoadDep *dep = load->first_dep;

		for (u32 i = 0; dep; dep = dep->next, i++)
			result.dependencies[i] = dep->handle;
	}

end:
	if (gltf)
		cgltf_free(gltf);

	ScratchRelease(&scratch);

	return result;
}

internal void
AST_ModelSerializerAlloc(const AST_Context *ctx,
						 AST_SerializerPipelineData *data,
						 AST_Asset *out)
{
	AST_ModelLoadData *load = data->data;
	GFX_Device *device = ctx->assets->device;

	osapi->MutexLock(ctx->assets->allocation_mutex);
	AST_SubModel *sub_models = ArenaPushArray(ctx->assets->arena, AST_SubModel, load->mesh_count);
	osapi->MutexUnlock(ctx->assets->allocation_mutex);

	AST_ModelLoadMesh *src = load->first_mesh;

	for (i32 i = (i32)load->mesh_count - 1; i >= 0; i--, src = src->next)
	{
		AST_SubModel *dst = &sub_models[i];

		dst->transform     = src->transform;

		dst->bounds_min    = src->bounds_min;
		dst->bounds_max    = src->bounds_max;

		dst->vertex_stride = sizeof(AST_ModelVertex);
		dst->index_stride  = sizeof(AST_ModelIndex);

		dst->vertex_count  = src->vertex_count;
		dst->index_count   = src->index_count;

		dst->material      = src->material;

		GFX_BufferAllocInfo vb_info = {0};
		vb_info.usage = VK_BUFFER_USAGE_2_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_2_TRANSFER_DST_BIT | VK_BUFFER_USAGE_2_TRANSFER_SRC_BIT;
		vb_info.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
		vb_info.size  = src->vertex_count * sizeof(AST_ModelVertex);

		GFX_BufferAllocInfo ib_info = {0};
		ib_info.usage = VK_BUFFER_USAGE_2_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_2_TRANSFER_DST_BIT | VK_BUFFER_USAGE_2_TRANSFER_SRC_BIT;
		ib_info.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
		ib_info.size  = src->index_count * sizeof(AST_ModelIndex);

		dst->vertex_buffer = GFX_DeviceBufferAlloc(device, &vb_info);
		dst->index_buffer  = GFX_DeviceBufferAlloc(device, &ib_info);
	}

	out->model.sub_model_count = load->mesh_count;
	out->model.sub_models = sub_models;
}

internal void
AST_ModelSerializerReload(const AST_Context *ctx,
						  AST_SerializerPipelineData *data,
						  AST_Asset *existing)
{
	DebugLogW(ctx->log_channel, "Reloading not implemented yet.");
}

internal void
AST_ModelSerializerGpu(const AST_Context *ctx,
					   AST_SerializerPipelineData *data,
					   AST_Asset *asset,
					   GFX_CmdBuffer *cmd,
					   GFX_BufferKey stage, u64 stage_base)
{
	AST_ModelLoadData *load = data->data;
	GFX_Device *device = ctx->assets->device;

	u64 offset = stage_base;

	AST_ModelLoadMesh **load_meshes = ArenaPushArray(ctx->scope, AST_ModelLoadMesh *, load->mesh_count);

	AST_ModelLoadMesh *src_mesh = load->first_mesh;

	for (i32 i = (i32)load->mesh_count - 1; i >= 0; i--, src_mesh = src_mesh->next)
	{
		load_meshes[i] = src_mesh;
	}

	for (u32 i = 0; i < asset->model.sub_model_count; i++)
	{
		AST_ModelLoadMesh *src = load_meshes[i];
		AST_SubModel      *dst = &asset->model.sub_models[i];

		u64 vb_size = src->vertex_count * sizeof(AST_ModelVertex);
		u64 ib_size = src->index_count  * sizeof(AST_ModelIndex);

		GFX_DeviceBufferWrite(device, stage, src->vertices, vb_size, offset);
		GFX_DeviceBufferWrite(device, stage, src->indices,  ib_size, offset + vb_size);

		GFX_BufferCopy vb_copy = {0};
		vb_copy.src_offset = offset;
		vb_copy.size = vb_size;

		GFX_BufferCopy ib_copy = {0};
		ib_copy.src_offset = offset + vb_size;
		ib_copy.size = ib_size;

		GFX_CmdCopyBufferToBuffer(cmd, stage, dst->vertex_buffer, 1, &vb_copy);
		GFX_CmdCopyBufferToBuffer(cmd, stage, dst->index_buffer,  1, &ib_copy);

		offset += vb_size + ib_size;
	}
}

internal void
AST_ModelSerializerDispose(AST_Asset *asset, AST_Assets *assets)
{
	GFX_Device *device = assets->device;

	for (u32 i = 0; i < asset->model.sub_model_count; i++)
	{
		GFX_DeviceBufferDestroy(device, asset->model.sub_models[i].vertex_buffer);
		GFX_DeviceBufferDestroy(device, asset->model.sub_models[i].index_buffer);
	}
}

internal AST_Serializer
AST_GetModelSerializer(void)
{
	static AST_Serializer model_serializer = {
		.Cpu     = AST_ModelSerializerCpu,
		.Alloc   = AST_ModelSerializerAlloc,
		.Reload  = AST_ModelSerializerReload,
		.Gpu     = AST_ModelSerializerGpu,
		.End     = NULL,
		.Dispose = AST_ModelSerializerDispose,
	};

	return model_serializer;
}
