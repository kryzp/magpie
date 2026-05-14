
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

internal v3
AST_GltfTransformTranslation(v3 v)
{
	// basis change
	return v3(v.x, -v.z, v.y);
}

internal v4
AST_GltfTransformQuat(v4 q)
{
	// basis change
	return v4(q.x, -q.z, q.y, q.w);
}

internal v3
AST_GltfTransformScale(v3 s)
{
	// basis change
	return v3(s.x, s.z, s.y);
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

	AST_ModelMaterial material;

	u32 vertex_count;
	AST_ModelVertex *vertices;

	u32 index_count;
	AST_ModelIndex *indices;

	AST_ModelSkinVertex *skin_vertices;
	i32 skin_index;
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

	u32 skeleton_count;
	AST_Skeleton *skeletons;

	u32 clip_count;
	AST_AnimClip *clips;

	u64 total_vertex_bytes;
	u64 total_index_bytes;
	u64 total_skin_vertex_bytes;
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
						 Arena *arena,
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
	String8 full_path = String8Append(arena, directory, relative);

	AST_Handle handle = AST_FromFilePath(ctx->assets, full_path);

	if (AST_IsValid(ctx->assets, handle))
		AST_ModelAddDependency(load, arena, handle);

	return handle;
}

internal AST_ModelMaterial
AST_ModelResolveMaterial(const AST_Context *ctx,
						 Arena *arena,
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

		mat.albedo_texture             = AST_ModelTryFetchTexture(ctx, arena, load, directory, &pbr->base_color_texture);
		mat.metallic_roughness_texture = AST_ModelTryFetchTexture(ctx, arena, load, directory, &pbr->metallic_roughness_texture);

		mat.albedo_factor      = v4(pbr->base_color_factor[0],
									pbr->base_color_factor[1],
									pbr->base_color_factor[2],
									pbr->base_color_factor[3]);
		
		mat.metallic_factor    = pbr->metallic_factor;
		mat.roughness_factor   = pbr->roughness_factor;
	}


	// NORMALS.
	mat.normal_texture = AST_ModelTryFetchTexture(ctx, arena, load, directory, &gltf_mat->normal_texture);
	mat.normal_scale   = gltf_mat->normal_texture.scale;


	// OCCLUSION.
	mat.occlusion_texture   = AST_ModelTryFetchTexture(ctx, arena, load, directory, &gltf_mat->occlusion_texture);
	mat.occlusion_intensity = gltf_mat->occlusion_texture.scale;


	// EMISSIVE.
	mat.emissive_texture = AST_ModelTryFetchTexture(ctx, arena, load, directory, &gltf_mat->emissive_texture);
	mat.emissive_factor  = v3(gltf_mat->emissive_factor[0],
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
		mat.transmission_texture = AST_ModelTryFetchTexture(ctx, arena, load, directory, &gltf_mat->transmission.transmission_texture);
		mat.transmission_factor  = gltf_mat->transmission.transmission_factor;
	}


	// VOLUME.
	if (gltf_mat->has_volume)
	{
		mat.thickness_texture    = AST_ModelTryFetchTexture(ctx, arena, load, directory, &gltf_mat->volume.thickness_texture);
		mat.thickness_factor     = gltf_mat->volume.thickness_factor;

		mat.attenuation_colour   = v3(gltf_mat->volume.attenuation_color[0],
									  gltf_mat->volume.attenuation_color[1],
									  gltf_mat->volume.attenuation_color[2]);

		mat.attenuation_distance = gltf_mat->volume.attenuation_distance;
	}


	// SPECULAR.
	if (gltf_mat->has_specular)
	{
		mat.specular_texture        = AST_ModelTryFetchTexture(ctx, arena, load, directory, &gltf_mat->specular.specular_texture);
		mat.specular_colour_texture = AST_ModelTryFetchTexture(ctx, arena, load, directory, &gltf_mat->specular.specular_color_texture);
		
		mat.specular_factor         = gltf_mat->specular.specular_factor;

		mat.specular_colour_factor  = v3(gltf_mat->specular.specular_color_factor[0],
										 gltf_mat->specular.specular_color_factor[1],
										 gltf_mat->specular.specular_color_factor[2]);
	}


	// CLEARCOAT.
	if (gltf_mat->has_clearcoat)
	{
		mat.clearcoat_texture           = AST_ModelTryFetchTexture(ctx, arena, load, directory, &gltf_mat->clearcoat.clearcoat_texture);
		mat.clearcoat_roughness_texture = AST_ModelTryFetchTexture(ctx, arena, load, directory, &gltf_mat->clearcoat.clearcoat_roughness_texture);

		mat.clearcoat_factor            = gltf_mat->clearcoat.clearcoat_factor;
		mat.clearcoat_roughness_factor  = gltf_mat->clearcoat.clearcoat_roughness_factor;
	}


	// SHEEN.
	if (gltf_mat->has_sheen)
	{
		mat.sheen_colour_texture    = AST_ModelTryFetchTexture(ctx, arena, load, directory, &gltf_mat->sheen.sheen_color_texture);
		mat.sheen_roughness_texture = AST_ModelTryFetchTexture(ctx, arena, load, directory, &gltf_mat->sheen.sheen_roughness_texture);

		mat.sheen_colour_factor     = v3(gltf_mat->sheen.sheen_color_factor[0],
										 gltf_mat->sheen.sheen_color_factor[1],
										 gltf_mat->sheen.sheen_color_factor[2]);

		mat.sheen_roughness_factor  = gltf_mat->sheen.sheen_roughness_factor;
	}


	// IRIDESCENCE.
	if (gltf_mat->has_iridescence)
	{
		mat.iridescence_texture                  = AST_ModelTryFetchTexture(ctx, arena, load, directory, &gltf_mat->iridescence.iridescence_texture);
		mat.iridescence_thickness_texture        = AST_ModelTryFetchTexture(ctx, arena, load, directory, &gltf_mat->iridescence.iridescence_thickness_texture);

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
						  Arena *arena,
						  AST_ModelLoadData *load,
						  String8 directory,
						  const cgltf_primitive *prim,
						  m4 world_transform,
						  const cgltf_skin *skin,
						  i32 skin_index)
{
	// TODO: support more topologies
	DebugLogAssert(ctx->log_channel,
				   prim->type == cgltf_primitive_type_triangles,
				   "Only support triangle-primitive models currently!");


	// Locate attributes.

	const cgltf_accessor *positions = NULL;
	const cgltf_accessor *normals   = NULL;
	const cgltf_accessor *tangents  = NULL;
	const cgltf_accessor *texcoords = NULL;
	const cgltf_accessor *colours   = NULL;
	const cgltf_accessor *joints    = NULL;
	const cgltf_accessor *weights   = NULL;

	for (u32 i = 0; i < prim->attributes_count; i++)
	{
		const cgltf_attribute *attr = &prim->attributes[i];

		switch (attr->type)
		{
			case cgltf_attribute_type_position:
				positions = attr->data;
				break;

			case cgltf_attribute_type_normal:
				normals = attr->data;
				break;
				
			case cgltf_attribute_type_tangent:
				tangents = attr->data;
				break;

			case cgltf_attribute_type_texcoord:
				if (attr->index == 0)
					texcoords = attr->data;
				break;

			case cgltf_attribute_type_color:
				if (attr->index == 0)
					colours = attr->data;
				break;

			case cgltf_attribute_type_joints:
				if (attr->index == 0)
					joints = attr->data;
				break;

			case cgltf_attribute_type_weights:
				if (attr->index == 0)
					weights = attr->data;
				break;

			default:
				break;
		}
	}

	if (!positions)
	{
		DebugLogW(ctx->log_channel, "Node has no positions??");
		return;
	}


	// Skinning.

	b32 is_skinned = ((skin != NULL) &&
					  (joints != NULL) &&
					  (weights != NULL));

	if ((skin != NULL) && (joints == NULL || weights == NULL))
	{
		DebugLogW(ctx->log_channel,
				  "Node has skin but missing joints and/or weights.");
	}
	

	// Vertices.

	u32 vert_count = (u32)positions->count;
	AST_ModelVertex *vertices = ArenaPushArray(arena, AST_ModelVertex, vert_count);
	AST_ModelSkinVertex *skin_vertices = NULL;

	if (is_skinned)
	{
		skin_vertices = ArenaPushArray(arena, AST_ModelSkinVertex, vert_count);

		DebugLogAssert(ctx->log_channel,
					   joints->count == positions->count &&
					   weights->count == positions->count,
					   "Joint / Weight count is in mismatch with position count in node.");
	}

	v3 bmin = v3( MATH_MAX_F32,  MATH_MAX_F32,  MATH_MAX_F32);
	v3 bmax = v3(-MATH_MAX_F32, -MATH_MAX_F32, -MATH_MAX_F32);

	for (u32 i = 0; i < vert_count; i++)
	{
		AST_ModelVertex *v = &vertices[i];

		f32 pos[3] = {0};
		cgltf_accessor_read_float(positions, i, pos, 3);
		v->position = AST_GltfTransformTranslation(v3(pos[0], pos[1], pos[2]));

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
			f32 col[4] = { 1.f, 1.f, 1.f, 1.f };
			u32 comps = (u32)cgltf_num_components(colours->type);

			if (comps > 4)
				comps = 4;

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
			v->normal = AST_GltfTransformTranslation(v3(n[0], n[1], n[2]));
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
			v3 T = AST_GltfTransformTranslation(v3(t[0], t[1], t[2]));
			f32 sign = t[3];

			v->tangent   = T;
			v->bitangent = v3((N.y * T.z - N.z * T.y) * sign,
							  (N.z * T.x - N.x * T.z) * sign,
							  (N.x * T.y - N.y * T.x) * sign);
		}
		else
		{
			// TODO: integrate using something called "mikktspace" for tangents.
			v->tangent   = v3x(0.f);
			v->bitangent = v3x(0.f);
		}

		if (is_skinned)
		{
			AST_ModelSkinVertex *sv = &skin_vertices[i];

			cgltf_uint j[4] = {0};
			cgltf_accessor_read_uint(joints, i, j, 4);

			DebugLogAssert(ctx->log_channel,
						   j[0] < skin->joints_count &&
						   j[1] < skin->joints_count &&
						   j[2] < skin->joints_count &&
						   j[3] < skin->joints_count,
						   "Joint index is out of range.");
			
			sv->joints[0] = j[0];
			sv->joints[1] = j[1];
			sv->joints[2] = j[2];
			sv->joints[3] = j[3];

			f32 w[4] = {0};
			cgltf_accessor_read_float(weights, i, w, 4);

			// renormalize 'cuz weights don't necessarily always
			// have to add up to 1 for whatever fucking reason.
			f32 wsum = w[0] + w[1] + w[2] + w[3];

			if (wsum > 0.f)
			{
				f32 inv = 1.f / wsum;

				w[0] *= inv;
				w[1] *= inv;
				w[2] *= inv;
				w[3] *= inv;
			}
			else
			{
				w[0] = 1.f; // zero influence so just bind to joint 0
			}

			sv->weights[0] = w[0];
			sv->weights[1] = w[1];
			sv->weights[2] = w[2];
			sv->weights[3] = w[3];
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
		material = AST_ModelResolveMaterial(ctx, arena, load, directory, prim->material);
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
	mesh->transform     = world_transform;
	mesh->bounds_min    = bmin;
	mesh->bounds_max    = bmax;
	mesh->vertex_count  = vert_count;
	mesh->vertices      = vertices;
	mesh->index_count   = idx_count;
	mesh->indices       = indices;
	mesh->skin_vertices = skin_vertices;
	mesh->skin_index    = skin_index;
	mesh->material      = material;

	mesh->next = load->first_mesh;
	load->first_mesh = mesh;
	load->mesh_count++;
	
	load->total_vertex_bytes += vert_count * sizeof(AST_ModelVertex);
	load->total_index_bytes  += idx_count  * sizeof(AST_ModelIndex);

	if (is_skinned)
		load->total_skin_vertex_bytes += vert_count * sizeof(AST_ModelSkinVertex);
}

internal void
AST_ModelProcessNode(const AST_Context *ctx,
					 Arena *arena,
					 AST_ModelLoadData *load,
					 String8 directory,
					 const cgltf_node *node,
					 const cgltf_data *gltf)
{
	if (node->mesh)
	{
		b32 is_skinned = node->skin != NULL;

		m4 mesh_transform = {0};

		if (is_skinned)
		{
			mesh_transform = M4Identity(); // glTf spec, node transform ignored, entirely based on joint.
		}
		else
		{
			cgltf_float world[16];
			cgltf_node_transform_world(node, world);
			mesh_transform = AST_GltfTransformM4(AST_GltfFloat16ToM4(world));
		}

		i32 skin_index = -1;

		if (is_skinned)
			skin_index = (i32)cgltf_skin_index(gltf, node->skin);

		for (u32 i = 0; i < node->mesh->primitives_count; i++)
		{
			const cgltf_primitive *prim = &node->mesh->primitives[i];
			AST_ModelProcessPrimitive(ctx, arena, load, directory, prim, mesh_transform, node->skin, skin_index);
		}
	}

	for (u32 i = 0; i < node->children_count; i++)
	{
		AST_ModelProcessNode(ctx, arena, load, directory, node->children[i], gltf);
	}
}

internal void
AST_ModelLoadSkeleton(const AST_Context *ctx,
					  Arena *arena,
					  AST_ModelLoadData *load,
					  const cgltf_data *gltf,
					  u32 index)
{
	const cgltf_skin *skin = &gltf->skins[index];
	AST_Skeleton *out = &load->skeletons[index];

	if (skin->name && skin->name[0])
		out->name = String8Clone(arena, String8FromCStr(skin->name));
	else if (skin->skeleton && skin->skeleton->name)
		out->name = String8Clone(arena, String8FromCStr(skin->skeleton->name));
	else
		out->name = String8Fmt(arena, "Unnamed Skeleton (%u)", index);
	
	out->joint_count = (u32)skin->joints_count;
	out->joints = ArenaPushArray(arena, AST_Joint, out->joint_count);

	for (u32 i = 0; i < out->joint_count; i++)
	{
		AST_Joint *j = &out->joints[i];

		const cgltf_node *node = skin->joints[i];

		if (node->name)
			j->name = String8Clone(arena, String8FromCStr(node->name));
		else
			j->name = String8Fmt(arena, "Unnamed Joint (%u)", i);

		j->parent = -1;

		if (node->parent)
		{
			for (u32 k = 0; k < out->joint_count; k++)
			{
				if (skin->joints[k] != node->parent)
					continue;

				j->parent = (i32)k;
				break;
			}
		}

		v3 translation = v3(0.f, 0.f, 0.f);
		v4 rotation    = v4(0.f, 0.f, 0.f, 1.f);
		v3 scale       = v3(1.f, 1.f, 1.f);

		if (node->has_matrix)
		{
			DebugLogW(ctx->log_channel, "Joint (%u) stores a baked matrix but I'm not bothering to decompose the matrix yet so we're just gonna assume identity and act like everything's fine :thumbsup:", i);
		}
		else
		{
			if (node->has_translation)
			{
				translation = v3(node->translation[0],
								 node->translation[1],
								 node->translation[2]);
			}

			if (node->has_rotation)
			{
				rotation = v4(node->rotation[0],
							  node->rotation[1],
							  node->rotation[2],
							  node->rotation[3]);
			}

			if (node->has_scale)
			{
				scale = v3(node->scale[0],
						   node->scale[1],
						   node->scale[2]);
			}
		}

		j->bind_translation = AST_GltfTransformTranslation(translation);
		j->bind_rotation    = AST_GltfTransformQuat(rotation);
		j->bind_scale       = AST_GltfTransformScale(scale);

		if (skin->inverse_bind_matrices)
		{
			f32 ibm[16] = {0};
			cgltf_accessor_read_float(skin->inverse_bind_matrices, i, ibm, 16);
			j->inverse_bind_matrix = AST_GltfTransformM4(AST_GltfFloat16ToM4(ibm));
		}
		else
		{
			j->inverse_bind_matrix = M4Identity();
		}
	}


	out->root_parent_world = M4Identity();
	
	const cgltf_node *top_joint_n = NULL;

	for (u32 i = 0; i < skin->joints_count; i++)
	{
		const cgltf_node *n = skin->joints[i];

		b32 is_parent = false;

		if (n->parent)
		{
			for (u32 k = 0; k < skin->joints_count; k++)
			{
				if (skin->joints[k] != n->parent)
					continue;

				is_parent = true;
				break;
			}
		}

		if (!is_parent)
		{
			top_joint_n = n;
			break;
		}
	}

	if (top_joint_n && top_joint_n->parent)
	{
		cgltf_float world[16] = {0};
		cgltf_node_transform_world(top_joint_n->parent, world);
		out->root_parent_world = AST_GltfTransformM4(AST_GltfFloat16ToM4(world));
	}
}

internal AST_AnimPath
AST_ModelAnimPathFromGltf(cgltf_animation_path_type t)
{
	switch (t)
	{
		case cgltf_animation_path_type_translation:  return AST_AnimPath_Translate;
		case cgltf_animation_path_type_rotation:     return AST_AnimPath_Rotation;
		case cgltf_animation_path_type_scale:        return AST_AnimPath_Scale;
	}

	AssertTrue(false);

	return AST_AnimPath_COUNT;
}

internal AST_AnimInterp
AST_ModelAnimInterpFromGltf(cgltf_interpolation_type t)
{
	switch (t)
	{
		case cgltf_interpolation_type_step:          return AST_AnimInterp_Step;
		case cgltf_interpolation_type_linear:        return AST_AnimInterp_Linear;
		case cgltf_interpolation_type_cubic_spline:  return AST_AnimInterp_Cubic;
	}

	AssertTrue(false);

	return AST_AnimInterp_COUNT;
}

/*
 * this function sucks dick holy shit
 */
internal void
AST_ModelLoadClip(const AST_Context *ctx,
				  Arena *arena,
				  AST_ModelLoadData *load,
				  const cgltf_data *gltf,
				  u32 index)
{
	AST_AnimClip *clip = &load->clips[index];

	const cgltf_animation *anim = &gltf->animations[index];

	if (anim->name)
		clip->name = String8Clone(arena, String8FromCStr(anim->name));
	else
		clip->name = String8Fmt(arena, "Animation Clip (%u)", index);

	u32 valid_count = 0;

	for (u32 j = 0; j < anim->channels_count; j++)
	{
		const cgltf_animation_channel *anim_ch = &anim->channels[j];

		if (!anim_ch->target_node || !anim_ch->sampler)
			continue;

		if (anim_ch->target_path == cgltf_animation_path_type_invalid)
			continue;

		if (anim_ch->target_path == cgltf_animation_path_type_weights)
			continue; // morph targets

		i32 joint_idx = -1;
		i32 skeleton_idx = -1;

		for (u32 ii = 0; ii < gltf->skins_count; ii++)
		{
			if (joint_idx >= 0)
				break;

			const cgltf_skin *s = &gltf->skins[ii];

			for (u32 jj = 0; jj < s->joints_count; jj++)
			{
				if (s->joints[jj] == anim_ch->target_node)
				{
					joint_idx = (i32)jj;
					skeleton_idx = (i32)ii;

					break;
				}
			}
		}

		if (joint_idx < 0)
			continue;

		valid_count++;
	}

	clip->duration_s = 0.f;

	clip->channel_count = valid_count;
	clip->channels = ArenaPushArray(arena, AST_AnimChannel, clip->channel_count);

	u32 curr = 0;

	for (u32 j = 0; j < anim->channels_count; j++)
	{
		const cgltf_animation_channel *anim_ch = &anim->channels[j];

		if (!anim_ch->target_node || !anim_ch->sampler)
			continue;

		if (anim_ch->target_path == cgltf_animation_path_type_invalid)
			continue;

		if (anim_ch->target_path == cgltf_animation_path_type_weights)
			continue; // morph targets

		i32 skeleton_idx = -1;
		i32 joint_idx = -1;

		for (u32 ii = 0; ii < gltf->skins_count; ii++)
		{
			if (joint_idx >= 0)
				break;

			const cgltf_skin *s = &gltf->skins[ii];

			for (u32 jj = 0; jj < s->joints_count; jj++)
			{
				if (s->joints[jj] == anim_ch->target_node)
				{
					joint_idx = (i32)jj;
					skeleton_idx = (i32)ii;

					break;
				}
			}
		}

		if (joint_idx < 0)
			continue;

		AST_AnimChannel *ch = &clip->channels[curr];
			
		const cgltf_animation_sampler *anim_sampler = anim_ch->sampler;

		ch->target_skeleton = skeleton_idx;
		ch->target_joint    = (u32)joint_idx;

		ch->path            = AST_ModelAnimPathFromGltf(anim_ch->target_path);
		ch->interp          = AST_ModelAnimInterpFromGltf(anim_sampler->interpolation);

		b32 cubic = anim_sampler->interpolation == cgltf_interpolation_type_cubic_spline;

		if (cubic)
		{
			DebugLogW(ctx->log_channel,
					  "Clip (%.*s) channel uses cubic interpolation but we don't support that yet so falling back to linear.",
					  (i32)clip->name.len, clip->name.str);

			ch->interp = AST_AnimInterp_Linear;
		}

		u32 nkeys = (u32)anim_sampler->input->count;

		ch->key_count = nkeys;
		ch->keys = ArenaPushArray(arena, AST_AnimKey, ch->key_count);

		for (u32 k = 0; k < ch->key_count; k++)
		{
			AST_AnimKey *key = &ch->keys[k];

			f32 t = 0.f;
			cgltf_accessor_read_float(anim_sampler->input, k, &t, 1);
			key->timestamp_s = t;

			switch (ch->path)
			{
				case AST_AnimPath_Translate:
					{
						f32 v[3] = { 0.f, 0.f, 0.f };
						cgltf_accessor_read_float(anim_sampler->output, k, v, 3);
						key->translation = AST_GltfTransformTranslation(v3(v[0], v[1], v[2]));
					}
					break;
						
				case AST_AnimPath_Rotation:
					{
						f32 v[4] = { 0.f, 0.f, 0.f, 1.f };
						cgltf_accessor_read_float(anim_sampler->output, k, v, 4);
						key->rotation = AST_GltfTransformQuat(v4(v[0], v[1], v[2], v[3]));	
					}
					break;

				case AST_AnimPath_Scale:
					{
						f32 v[3] = { 1.f, 1.f, 1.f };
						cgltf_accessor_read_float(anim_sampler->output, k, v, 3);
						key->scale = AST_GltfTransformScale(v3(v[0], v[1], v[2]));
					}
					break;
			}
		}

		if (nkeys > 0 && ch->keys[nkeys - 1].timestamp_s > clip->duration_s)
			clip->duration_s = ch->keys[nkeys - 1].timestamp_s;
			
		curr++;
	}
}

internal AST_SerializerPipelineData
AST_ModelSerializerCpu(const AST_Context *ctx, Arena *load_scope)
{
	ScratchArena scratch = ScratchBegin(NULL, 0);

	String8 file_path = AST_ContextSystemFilePath(ctx, scratch.arena);

	AST_ModelLoadData *load = ArenaPushArray(load_scope, AST_ModelLoadData, 1);
	MemZeroStruct(load);

	AST_SerializerPipelineData result = {0};
	result.data = load;

	cgltf_options options = {0};
	cgltf_data *gltf = NULL;

	if (cgltf_parse_file(&options, (const char *)file_path.str, &gltf) != cgltf_result_success)
	{
		DebugLogE(ctx->log_channel,
				  "Failed to parse glTF model: %.*s",
				  (i32)file_path.len,
				  file_path.str);
		
		result.failed = true;
		goto end;
	}

	if (cgltf_load_buffers(&options, gltf, (const char *)file_path.str) != cgltf_result_success)
	{
		DebugLogE(ctx->log_channel,
				  "Failed to load buffers of glTF model: %.*s",
				  (i32)file_path.len,
				  file_path.str);
		
		result.failed = true;
		goto end;
	}

	if (cgltf_validate(gltf) != cgltf_result_success)
	{
		DebugLogE(ctx->log_channel,
				  "Failed to validate glTF model: %.*s",
				  (i32)file_path.len,
				  file_path.str);
		
		result.failed = true;
		goto end;
	}

	String8 directory = IO_PathGetFileDirectory(scratch.arena, ctx->metadata.path);

	const cgltf_scene *scene = gltf->scene;

	if (!scene && gltf->scenes_count > 0)
		scene = &gltf->scenes[0];

	if (!scene)
	{
		DebugLogE(ctx->log_channel,
				  "Failed to create cgltf scene.");
		
		result.failed = true;
		goto end;
	}

	for (u32 i = 0; i < scene->nodes_count; i++)
		AST_ModelProcessNode(ctx, load_scope, load, directory, scene->nodes[i], gltf);
	
	if (gltf->skins_count > 0)
	{
		load->skeleton_count = (u32)gltf->skins_count;
		load->skeletons = ArenaPushArray(load_scope, AST_Skeleton, load->skeleton_count);

		for (u32 i = 0; i < load->skeleton_count; i++)
			AST_ModelLoadSkeleton(ctx, load_scope, load, gltf, i);
		
		load->clip_count = (u32)gltf->animations_count;
		load->clips = ArenaPushArray(load_scope, AST_AnimClip, load->clip_count);

		for (u32 i = 0; i < load->clip_count; i++)
			AST_ModelLoadClip(ctx, load_scope, load, gltf, i);
	}
	
	result.stage_size = load->total_vertex_bytes + load->total_index_bytes + load->total_skin_vertex_bytes;
	result.failed = false;

	result.dependency_count = load->dep_count;

	if (load->dep_count > 0)
	{
		result.dependencies = ArenaPushArray(load_scope, AST_Handle, load->dep_count);

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
						 AST_Asset *out,
						 Arena *arena)
{
	AST_ModelLoadData *load = data->data;
	GFX_Device *device = ctx->assets->device;

	
	AST_SubModel *sub_models = ArenaPushArray(arena, AST_SubModel, load->mesh_count);
	
	AST_ModelLoadMesh *src_mesh = load->first_mesh;

	for (i32 i = (i32)load->mesh_count - 1; i >= 0; i--, src_mesh = src_mesh->next)
	{
		AST_SubModel *dst = &sub_models[i];

		dst->transform     = src_mesh->transform;

		dst->bounds_min    = src_mesh->bounds_min;
		dst->bounds_max    = src_mesh->bounds_max;

		dst->material      = src_mesh->material;

		dst->vertex_stride = sizeof(AST_ModelVertex);
		dst->index_stride  = sizeof(AST_ModelIndex);

		dst->vertex_count  = src_mesh->vertex_count;
		dst->index_count   = src_mesh->index_count;

		GFX_BufferAllocInfo vb_info = {0};
		vb_info.usage = VK_BUFFER_USAGE_2_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_2_TRANSFER_DST_BIT | VK_BUFFER_USAGE_2_TRANSFER_SRC_BIT;
		vb_info.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
		vb_info.size  = src_mesh->vertex_count * sizeof(AST_ModelVertex);

		GFX_BufferAllocInfo ib_info = {0};
		ib_info.usage = VK_BUFFER_USAGE_2_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_2_TRANSFER_DST_BIT | VK_BUFFER_USAGE_2_TRANSFER_SRC_BIT;
		ib_info.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
		ib_info.size  = src_mesh->index_count * sizeof(AST_ModelIndex);

		dst->vertex_buffer = GFX_DeviceBufferAlloc(device, &vb_info);
		dst->index_buffer  = GFX_DeviceBufferAlloc(device, &ib_info);

		if (src_mesh->skin_vertices)
		{
			GFX_BufferAllocInfo svb_info = {0};
			svb_info.usage = VK_BUFFER_USAGE_2_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_2_TRANSFER_DST_BIT | VK_BUFFER_USAGE_2_TRANSFER_SRC_BIT;
			svb_info.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
			svb_info.size  = src_mesh->vertex_count * sizeof(AST_ModelSkinVertex);
			
			dst->skin_buffer = GFX_DeviceBufferAlloc(device, &svb_info);
		}
		else
		{
			dst->skin_buffer = GFX_BufferKeyNull();
		}
		
		dst->skin_index = src_mesh->skin_index;
	}

	
	AST_Skeleton *skeletons = NULL;

	if (load->skeleton_count > 0)
		skeletons = ArenaPushArray(arena, AST_Skeleton, load->skeleton_count);

	for (u32 i = 0; i < load->skeleton_count; i++)
	{
		AST_Skeleton *src = &load->skeletons[i];
		AST_Skeleton *dst = &skeletons[i];

		dst->name = String8Clone(arena, src->name);
		
		dst->joint_count = src->joint_count;
		dst->joints = ArenaPushArray(arena, AST_Joint, src->joint_count);

		dst->root_parent_world = src->root_parent_world;

		for (u32 j = 0; j < dst->joint_count; j++)
		{
			AST_Joint *src_j = &src->joints[j];
			AST_Joint *dst_j = &dst->joints[j];
			
			dst_j->name                = String8Clone(arena, src_j->name);
			
			dst_j->parent              = src_j->parent;

			dst_j->bind_translation    = src_j->bind_translation;
			dst_j->bind_rotation       = src_j->bind_rotation;
			dst_j->bind_scale          = src_j->bind_scale;

			dst_j->inverse_bind_matrix = src_j->inverse_bind_matrix;
		}
	}

	
	AST_AnimClip *clips = NULL;

	if (load->clip_count)
		clips = ArenaPushArray(arena, AST_AnimClip, load->clip_count);

	for (u32 i = 0; i < load->clip_count; i++)
	{
		AST_AnimClip *src = &load->clips[i];
		AST_AnimClip *dst = &clips[i];

		dst->name = String8Clone(arena, src->name);
		
		dst->duration_s = src->duration_s;

		dst->channel_count = src->channel_count;
		dst->channels = ArenaPushArray(arena, AST_AnimChannel, src->channel_count);

		for (u32 j = 0; j < dst->channel_count; j++)
		{
			AST_AnimChannel *src_ch = &src->channels[j];
			AST_AnimChannel *dst_ch = &dst->channels[j];

			dst_ch->target_skeleton = src_ch->target_skeleton;
			dst_ch->target_joint    = src_ch->target_joint;

			dst_ch->path            = src_ch->path;
			dst_ch->interp          = src_ch->interp;
			
			dst_ch->key_count       = src_ch->key_count;
			dst_ch->keys            = ArenaPushArray(arena, AST_AnimKey, dst_ch->key_count);

			for (u32 k = 0; k < dst_ch->key_count; k++)
				dst_ch->keys[k] = src_ch->keys[k];
		}
	}
	
	
	out->model.sub_model_count = load->mesh_count;
	out->model.sub_models = sub_models;

	out->model.skeleton_count = load->skeleton_count;
	out->model.skeletons = skeletons;

	out->model.clip_count = load->clip_count;
	out->model.clips = clips;
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

	ScratchArena scratch = ScratchBegin(NULL, 0);
	
	AST_ModelLoadMesh **load_meshes = ArenaPushArray(scratch.arena, AST_ModelLoadMesh *, load->mesh_count);

	AST_ModelLoadMesh *src_mesh = load->first_mesh;

	for (i32 i = (i32)load->mesh_count - 1; i >= 0; i--, src_mesh = src_mesh->next)
	{
		load_meshes[i] = src_mesh;
	}

	for (u32 i = 0; i < asset->model.sub_model_count; i++)
	{
		AST_ModelLoadMesh *src = load_meshes[i];
		AST_SubModel      *dst = &asset->model.sub_models[i];


		// Vertices.
		
		u64 vb_size = src->vertex_count * sizeof(AST_ModelVertex);
		
		GFX_DeviceBufferWrite(device, stage, src->vertices, vb_size,  offset);
		
		GFX_BufferCopy vb_copy = {0};
		vb_copy.src_offset = offset;
		vb_copy.size = vb_size;
		
		GFX_CmdCopyBufferToBuffer(cmd, stage, dst->vertex_buffer, 1, &vb_copy);

		offset += vb_size;


		// Indices.
		
		u64 ib_size = src->index_count * sizeof(AST_ModelIndex);

		GFX_DeviceBufferWrite(device, stage, src->indices, ib_size,  offset);

		GFX_BufferCopy ib_copy = {0};
		ib_copy.src_offset = offset;
		ib_copy.size = ib_size;

		GFX_CmdCopyBufferToBuffer(cmd, stage, dst->index_buffer,  1, &ib_copy);
		
		offset += ib_size;


		// Skinning.
		
		if (src->skin_vertices)
		{
			u64 svb_size = src->vertex_count * sizeof(AST_ModelSkinVertex);
			
			GFX_DeviceBufferWrite(device, stage, src->skin_vertices, svb_size, offset);

			GFX_BufferCopy svb_copy = {0};
			svb_copy.src_offset = offset;
			svb_copy.size = svb_size;

			GFX_CmdCopyBufferToBuffer(cmd, stage, dst->skin_buffer, 1, &svb_copy);

			offset += svb_size;
		}
	}

	ScratchRelease(&scratch);
}

internal void
AST_ModelSerializerDispose(AST_Asset *asset, AST_Assets *assets)
{
	GFX_Device *device = assets->device;

	for (u32 i = 0; i < asset->model.sub_model_count; i++)
	{
		AST_SubModel *sub_model = &asset->model.sub_models[i];
		
		GFX_DeviceBufferDestroy(device, sub_model->vertex_buffer);
		GFX_DeviceBufferDestroy(device, sub_model->index_buffer);

		if (!GFX_BufferKeyIsNull(sub_model->skin_buffer))
			GFX_DeviceBufferDestroy(device, sub_model->skin_buffer);
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
