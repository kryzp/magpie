
const global struct aiMatrix4x4 ast_model_assimp_basis = {
	1.f, 0.f, 0.f, 0.f,
	0.f, 0.f,-1.f, 0.f,
	0.f, 1.f, 0.f, 0.f,
	0.f, 0.f, 0.f, 1.f
};

internal m4
AST_aiMatrix4x4ToM4(struct aiMatrix4x4 m)
{
	return (m4) {
		m.a1, m.b1, m.c1, m.d1,
		m.a2, m.b2, m.c2, m.d2,
		m.a3, m.b3, m.c3, m.d3,
		m.a4, m.b4, m.c4, m.d4
	};
}

internal v3
AST_aiTransformV3(const struct aiMatrix4x4 *m, struct aiVector3D v)
{
	v3 w = {0};
	
	w.x = (m->a1 * v.x) + (m->a2 * v.y) + (m->a3 * v.z);
	w.y = (m->b1 * v.x) + (m->b2 * v.y) + (m->b3 * v.z);
	w.z = (m->c1 * v.x) + (m->c2 * v.y) + (m->c3 * v.z);

	return w;
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
						 const struct aiMaterial *ai_mat,
						 enum aiTextureType type)
{
	if (aiGetMaterialTextureCount(ai_mat, type) == 0)
		return AST_HandleNull();

	struct aiString ai_path = {0};
	
	aiGetMaterialTexture(ai_mat, type, 0,
						 &ai_path,
						 NULL, NULL, NULL, NULL, NULL, NULL);

	String8 relative = String8Init(ai_path.data, ai_path.length);
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
						 const struct aiMaterial *ai_mat)
{
	AST_ModelMaterial mat = {0};

	mat.albedo             = AST_ModelTryFetchTexture(ctx, load, directory, ai_mat, aiTextureType_DIFFUSE);
	mat.normal             = AST_ModelTryFetchTexture(ctx, load, directory, ai_mat, aiTextureType_NORMALS);
	mat.emissive           = AST_ModelTryFetchTexture(ctx, load, directory, ai_mat, aiTextureType_EMISSIVE);
	mat.metallic_roughness = AST_ModelTryFetchTexture(ctx, load, directory, ai_mat, aiTextureType_DIFFUSE_ROUGHNESS);
	mat.ambient            = AST_ModelTryFetchTexture(ctx, load, directory, ai_mat, aiTextureType_LIGHTMAP);

	mat.albedo_factor      = v4(1.f, 1.f, 1.f, 1.f);
	mat.metallic_factor    = 1.f;
	mat.roughness_factor   = 1.f;
	mat.emissive_factor    = 0.f;
	
	mat.double_sided       = false;
	
	struct aiColor4D colour = {0};
	
	if (aiGetMaterialColor(ai_mat, AI_MATKEY_COLOR_DIFFUSE, &colour) == AI_SUCCESS)
		mat.albedo_factor = v4(colour.r, colour.g, colour.b, colour.a);

	aiGetMaterialFloat(ai_mat, AI_MATKEY_OPACITY, &mat.albedo_factor.w);
	aiGetMaterialFloat(ai_mat, AI_MATKEY_METALLIC_FACTOR, &mat.metallic_factor);
	aiGetMaterialFloat(ai_mat, AI_MATKEY_ROUGHNESS_FACTOR, &mat.roughness_factor);
	aiGetMaterialFloat(ai_mat, AI_MATKEY_EMISSIVE_INTENSITY, &mat.emissive_factor);
	
	i32 double_sided = 0;
	
	if (aiGetMaterialInteger(ai_mat, AI_MATKEY_TWOSIDED, &double_sided) == AI_SUCCESS)
		mat.double_sided = !!double_sided;

	return mat;
}

internal void
AST_ModelProcessMesh(const AST_Context *ctx,
					 AST_ModelLoadData *load,
					 String8 directory,
					 const struct aiMesh *ai_mesh,
					 const struct aiScene *ai_scene,
					 struct aiMatrix4x4 transform)
{
	Arena *arena = ctx->scope;

	
	// Vertices
	
	u32 vert_count = ai_mesh->mNumVertices;
	AST_ModelVertex *vertices = ArenaPushArray(arena, AST_ModelVertex, vert_count);

	v3 bmin = v3( MATH_MAX_F32,  MATH_MAX_F32,  MATH_MAX_F32);
	v3 bmax = v3(-MATH_MAX_F32, -MATH_MAX_F32, -MATH_MAX_F32);

	for (u32 i = 0; i < vert_count; i++)
	{
		AST_ModelVertex *v = &vertices[i];

		if (ai_mesh->mVertices)
		{
			v->position = AST_aiTransformV3(&ast_model_assimp_basis, ai_mesh->mVertices[i]);

			bmin = V3MinOf(bmin, v->position);
			bmax = V3MaxOf(bmax, v->position);
		}
		else
		{
			v->position = v3x(0.f);
		}

		if (ai_mesh->mTextureCoords[0])
		{
			struct aiVector3D uv = ai_mesh->mTextureCoords[0][i];
			v->texcoord = v2(uv.x, uv.y);
		}
		else
		{
			v->texcoord = v2x(0.f);
		}

		if (ai_mesh->mColors[0])
		{
			struct aiColor4D c = ai_mesh->mColors[0][i];
			v->colour = v3(c.r, c.g, c.b);
		}
		else
		{
			v->colour = v3x(1.f);
		}

		if (ai_mesh->mNormals)
		{
			v->normal = AST_aiTransformV3(&ast_model_assimp_basis, ai_mesh->mNormals[i]);
		}
		else
		{
			v->normal = v3x(0.f);
		}

		if (ai_mesh->mTangents)
		{
			v->tangent   = AST_aiTransformV3(&ast_model_assimp_basis, ai_mesh->mTangents[i]);
			v->bitangent = AST_aiTransformV3(&ast_model_assimp_basis, ai_mesh->mBitangents[i]);
		}
		else
		{
			v->tangent   = v3x(0.f);
			v->bitangent = v3x(0.f);
		}
	}
	

	// Indices

	u32 idx_count = 0;

	for (u32 i = 0; i < ai_mesh->mNumFaces; i++)
		idx_count += ai_mesh->mFaces[i].mNumIndices;

	AST_ModelIndex *indices = ArenaPushArray(arena, AST_ModelIndex, idx_count);
	u32 curr_index = 0;

	for (u32 i = 0; i < ai_mesh->mNumFaces; i++)
	{
		struct aiFace *face = &ai_mesh->mFaces[i];

		for (u32 j = 0; j < face->mNumIndices; j++)
			indices[curr_index++] = face->mIndices[j];
	}


	// Material
	
	const struct aiMaterial *ai_mat = ai_scene->mMaterials[ai_mesh->mMaterialIndex];
	AST_ModelMaterial material = AST_ModelResolveMaterial(ctx, load, directory, ai_mat);


	// Push onto list
	
	AST_ModelLoadMesh *mesh = ArenaPushArray(arena, AST_ModelLoadMesh, 1);
	mesh->next = load->first_mesh;
	load->first_mesh = mesh;
	load->mesh_count++;

	mesh->transform  = AST_aiMatrix4x4ToM4(transform);
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
AST_ModelProcessNodes(const AST_Context *ctx,
					  AST_ModelLoadData *load,
					  String8 directory,
					  const struct aiNode *node,
					  const struct aiScene *scene,
					  struct aiMatrix4x4 parent_transform)
{
	struct aiMatrix4x4 current = parent_transform;
	
	aiMultiplyMatrix4(&current, &node->mTransformation);

	for (u32 i = 0; i < node->mNumMeshes; i++)
	{
		const struct aiMesh *ai_mesh = scene->mMeshes[node->mMeshes[i]];
		AST_ModelProcessMesh(ctx, load, directory, ai_mesh, scene, current);
	}

	for (u32 i = 0; i < node->mNumChildren; i++)
	{
		AST_ModelProcessNodes(ctx, load, directory, node->mChildren[i], scene, current);
	}
}

internal AST_SerializerPipelineData
AST_ModelSerializerCpu(const AST_Context *ctx)
{
	ScratchArena scratch = ScratchBegin(&ctx->scope, 1);

	String8 file_path = AST_ContextSystemFilePath(ctx, scratch.arena);

	const struct aiScene *scene = aiImportFile((const char *)file_path.str,
											   aiProcess_Triangulate |
											   aiProcess_CalcTangentSpace |
											   aiProcess_FlipUVs |
											   aiProcess_JoinIdenticalVertices |
											   aiProcess_GenSmoothNormals);

	AST_ModelLoadData *load = ArenaPushArray(ctx->scope, AST_ModelLoadData, 1);
	MemZeroStruct(load);

	AST_SerializerPipelineData result = {0};
	result.data = load;

	b32 failed = !scene ||
		(scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) ||
		!scene->mRootNode;

	if (failed)
	{
		result.failed = true;
		goto end;
	}


	
	u64 last_slash = String8FindLast(file_path, String8Lit("/"));

	if (last_slash == CORE_STRING_INVALID_INDEX)
		last_slash = String8FindLast(file_path, String8Lit("\\"));

	String8 directory = String8Substr(file_path, 0, last_slash + 1);

	struct aiMatrix4x4 identity = {0};
	aiIdentityMatrix4(&identity);

	AST_ModelProcessNodes(ctx, load, directory, scene->mRootNode, scene, identity);


	
	result.stage_size = load->total_vertex_bytes + load->total_index_bytes;
	result.failed = false;

	// flatten dependency linked list into array for the pipeline.
	result.dependency_count = load->dep_count;

	if (load->dep_count > 0)
	{
		result.dependencies = ArenaPushArray(ctx->scope, AST_Handle, load->dep_count);

		AST_ModelLoadDep *dep = load->first_dep;

		for (u32 i = 0; dep; dep = dep->next, i++)
			result.dependencies[i] = dep->handle;
	}

end:
	if (scene)
		aiReleaseImport(scene);

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
	// TODO
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
		u64 ib_size = src->index_count  * sizeof(u32);

		GFX_DeviceBufferWrite(device, stage, src->vertices, vb_size, offset);
		GFX_DeviceBufferWrite(device, stage, src->indices,  ib_size, offset + vb_size);

		VkBufferCopy2 vb_copy = {0};
		vb_copy.sType = VK_STRUCTURE_TYPE_BUFFER_COPY_2;
		vb_copy.srcOffset = offset;
		vb_copy.size = vb_size;

		VkBufferCopy2 ib_copy = {0};
		ib_copy.sType = VK_STRUCTURE_TYPE_BUFFER_COPY_2;
		ib_copy.srcOffset = offset + vb_size;
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
