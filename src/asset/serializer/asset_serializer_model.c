
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
A_GltfFloat16ToM4(const cgltf_float m[16])
{
	return (m4) {
		m[0],  m[1],  m[2],  m[3],
		m[4],  m[5],  m[6],  m[7],
		m[8],  m[9],  m[10], m[11],
		m[12], m[13], m[14], m[15]
	};
}

internal v3
A_GltfTransformTranslation(v3 v)
{
	// basis change
	return v3(v.x, -v.z, v.y);
}

internal v4
A_GltfTransformQuat(v4 q)
{
	// basis change
	return v4(q.x, -q.z, q.y, q.w);
}

internal v3
A_GltfTransformScale(v3 s)
{
	// basis change
	return v3(s.x, s.z, s.y);
}

internal m4
A_GltfTransformM4(m4 m)
{
	// magpie = basis * cgltf * basis^-1
	return M4MulM4(ast_gltf_basis, M4MulM4(m, ast_gltf_basis_inv));
}

typedef struct A_ModelLoadMesh A_ModelLoadMesh;
struct A_ModelLoadMesh
{
	A_ModelLoadMesh *next;

	m4 transform;

	v3 bounds_min;
	v3 bounds_max;

	A_ModelMaterial material;

	u32 vertex_count;
	A_ModelVertex *vertices;

	u32 index_count;
	A_ModelIndex *indices;

	A_ModelSkinVertex *skin_vertices;
	i32 skin_index;
};

typedef struct A_ModelLoadDep A_ModelLoadDep;
struct A_ModelLoadDep
{
	A_ModelLoadDep *next;
	A_Handle handle;
};

typedef struct A_ModelLoadData A_ModelLoadData;
struct A_ModelLoadData
{
	A_ModelLoadMesh *first_mesh;
	u32 mesh_count;

	A_ModelLoadDep *first_dep;
	u32 dep_count;

	u32 skeleton_count;
	A_Skeleton *skeletons;

	u32 clip_count;
	A_AnimClip *clips;

	u64 total_vertex_bytes;
	u64 total_index_bytes;
	u64 total_skin_vertex_bytes;
};

internal void
A_ModelAddDependency(A_ModelLoadData *load, Arena *arena, A_Handle handle)
{
	A_ModelLoadDep *dep = ArenaPushArray(arena, A_ModelLoadDep, 1);
	dep->handle = handle;
	dep->next = load->first_dep;
	load->first_dep = dep;
	load->dep_count++;
}

internal A_Handle
A_ModelTryFetchTexture(const A_Context *ctx,
						 Arena *arena,
						 A_ModelLoadData *load,
						 String8 directory,
						 const cgltf_texture_view *view)
{
	if (!view->texture || !view->texture->image)
		return A_HandleNull();

	const cgltf_image *image = view->texture->image;
	
	// TODO: handle data: URIs (base64 embedded)
	//       and bufferview-backed images (.glb).
	if (!image->uri)
	{
		DebugLogB(ctx->log_channel, "URIs not supported yet.");
		return A_HandleNull();
	}

	if (strncmp(image->uri, "data:", 5) == 0)
		return A_HandleNull();

	String8 relative = String8Init(image->uri, strlen(image->uri));
	String8 full_path = String8Append(arena, directory, relative);

	A_Handle handle = A_FromFilePath(ctx->assets, full_path, A_Type_Texture);

	if (A_IsValid(ctx->assets, handle))
		A_ModelAddDependency(load, arena, handle);

	return handle;
}

internal A_ModelMaterial
A_ModelResolveMaterial(const A_Context *ctx,
						 Arena *arena,
						 A_ModelLoadData *load,
						 String8 directory,
						 const cgltf_material *gltf_mat)
{
	A_ModelMaterial mat = {0};

	if (gltf_mat->name && gltf_mat->name[0])
		mat.name = String8Clone(arena, String8FromCStr(gltf_mat->name));
	else
		mat.name = String8Fmt(arena, "Unnamed Material");

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


	// STANDARD METALLIC-ROUGHNESS OPAQUE PBR.
	if (gltf_mat->has_pbr_metallic_roughness)
	{
		const cgltf_pbr_metallic_roughness *pbr = &gltf_mat->pbr_metallic_roughness;

		mat.albedo_texture             = A_ModelTryFetchTexture(ctx, arena, load, directory, &pbr->base_color_texture);
		mat.metallic_roughness_texture = A_ModelTryFetchTexture(ctx, arena, load, directory, &pbr->metallic_roughness_texture);

		mat.albedo_factor      = v4(pbr->base_color_factor[0],
									pbr->base_color_factor[1],
									pbr->base_color_factor[2],
									pbr->base_color_factor[3]);
		
		mat.metallic_factor    = pbr->metallic_factor;
		mat.roughness_factor   = pbr->roughness_factor;
	}


	// NORMALS.
	mat.normal_texture = A_ModelTryFetchTexture(ctx, arena, load, directory, &gltf_mat->normal_texture);
	mat.normal_scale   = gltf_mat->normal_texture.scale;


	// OCCLUSION.
	mat.occlusion_texture   = A_ModelTryFetchTexture(ctx, arena, load, directory, &gltf_mat->occlusion_texture);
	mat.occlusion_intensity = gltf_mat->occlusion_texture.scale;


	// EMISSIVE.
	mat.emissive_texture = A_ModelTryFetchTexture(ctx, arena, load, directory, &gltf_mat->emissive_texture);
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
		mat.transmission_texture = A_ModelTryFetchTexture(ctx, arena, load, directory, &gltf_mat->transmission.transmission_texture);
		mat.transmission_factor  = gltf_mat->transmission.transmission_factor;
	}


	// VOLUME.
	if (gltf_mat->has_volume)
	{
		mat.thickness_texture    = A_ModelTryFetchTexture(ctx, arena, load, directory, &gltf_mat->volume.thickness_texture);
		mat.thickness_factor     = gltf_mat->volume.thickness_factor;

		mat.attenuation_colour   = v3(gltf_mat->volume.attenuation_color[0],
									  gltf_mat->volume.attenuation_color[1],
									  gltf_mat->volume.attenuation_color[2]);

		mat.attenuation_distance = gltf_mat->volume.attenuation_distance;
	}


	// SPECULAR.
	if (gltf_mat->has_specular)
	{
		mat.specular_texture        = A_ModelTryFetchTexture(ctx, arena, load, directory, &gltf_mat->specular.specular_texture);
		mat.specular_colour_texture = A_ModelTryFetchTexture(ctx, arena, load, directory, &gltf_mat->specular.specular_color_texture);
		
		mat.specular_factor         = gltf_mat->specular.specular_factor;

		mat.specular_colour_factor  = v3(gltf_mat->specular.specular_color_factor[0],
										 gltf_mat->specular.specular_color_factor[1],
										 gltf_mat->specular.specular_color_factor[2]);
	}


	// CLEARCOAT.
	if (gltf_mat->has_clearcoat)
	{
		mat.clearcoat_texture           = A_ModelTryFetchTexture(ctx, arena, load, directory, &gltf_mat->clearcoat.clearcoat_texture);
		mat.clearcoat_roughness_texture = A_ModelTryFetchTexture(ctx, arena, load, directory, &gltf_mat->clearcoat.clearcoat_roughness_texture);

		mat.clearcoat_factor            = gltf_mat->clearcoat.clearcoat_factor;
		mat.clearcoat_roughness_factor  = gltf_mat->clearcoat.clearcoat_roughness_factor;
	}


	// SHEEN.
	if (gltf_mat->has_sheen)
	{
		mat.sheen_colour_texture    = A_ModelTryFetchTexture(ctx, arena, load, directory, &gltf_mat->sheen.sheen_color_texture);
		mat.sheen_roughness_texture = A_ModelTryFetchTexture(ctx, arena, load, directory, &gltf_mat->sheen.sheen_roughness_texture);

		mat.sheen_colour_factor     = v3(gltf_mat->sheen.sheen_color_factor[0],
										 gltf_mat->sheen.sheen_color_factor[1],
										 gltf_mat->sheen.sheen_color_factor[2]);

		mat.sheen_roughness_factor  = gltf_mat->sheen.sheen_roughness_factor;
	}


	// IRIDESCENCE.
	if (gltf_mat->has_iridescence)
	{
		mat.iridescence_texture                  = A_ModelTryFetchTexture(ctx, arena, load, directory, &gltf_mat->iridescence.iridescence_texture);
		mat.iridescence_thickness_texture        = A_ModelTryFetchTexture(ctx, arena, load, directory, &gltf_mat->iridescence.iridescence_thickness_texture);

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
		case cgltf_alpha_mode_opaque: mat.alpha_mode = A_AlphaMode_Opaque; break;
		case cgltf_alpha_mode_mask:   mat.alpha_mode = A_AlphaMode_Mask;   break;
		case cgltf_alpha_mode_blend:  mat.alpha_mode = A_AlphaMode_Blend;  break;
		default:                      mat.alpha_mode = A_AlphaMode_Opaque; break;
	}

	mat.alpha_cutoff = gltf_mat->alpha_cutoff;

	DebugLogT(ctx->log_channel, "Loaded Material: %.*s", String8VArg(mat.name));
	
	return mat;
}

internal void
A_ModelProcessPrimitive(const A_Context *ctx,
						  Arena *arena,
						  A_ModelLoadData *load,
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
	A_ModelVertex *vertices = ArenaPushArray(arena, A_ModelVertex, vert_count);
	A_ModelSkinVertex *skin_vertices = NULL;

	if (is_skinned)
	{
		skin_vertices = ArenaPushArray(arena, A_ModelSkinVertex, vert_count);

		DebugLogAssert(ctx->log_channel,
					   joints->count == positions->count &&
					   weights->count == positions->count,
					   "Joint / Weight count is in mismatch with position count in node.");
	}

	v3 bmin = v3( MATH_MAX_F32,  MATH_MAX_F32,  MATH_MAX_F32);
	v3 bmax = v3(-MATH_MAX_F32, -MATH_MAX_F32, -MATH_MAX_F32);

	for (u32 i = 0; i < vert_count; i++)
	{
		A_ModelVertex *v = &vertices[i];

		f32 pos[3] = {0};
		cgltf_accessor_read_float(positions, i, pos, 3);
		v->position = A_GltfTransformTranslation(v3(pos[0], pos[1], pos[2]));

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
			v->normal = A_GltfTransformTranslation(v3(n[0], n[1], n[2]));
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
			v3 T = A_GltfTransformTranslation(v3(t[0], t[1], t[2]));
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
			A_ModelSkinVertex *sv = &skin_vertices[i];

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
	A_ModelIndex *indices = NULL;

	if (prim->indices)
	{
		idx_count = (u32)prim->indices->count;
		indices = ArenaPushArray(arena, A_ModelIndex, idx_count);

		for (u32 i = 0; i < idx_count; i++)
		{
			cgltf_uint idx = 0;
			cgltf_accessor_read_uint(prim->indices, i, &idx, 1);
			indices[i] = (A_ModelIndex)idx;
		}
	}
	else
	{
		// Non-indexed primitive: emit sequential indices.
		idx_count = vert_count;
		indices = ArenaPushArray(arena, A_ModelIndex, idx_count);

		for (u32 i = 0; i < idx_count; i++)
			indices[i] = (A_ModelIndex)i;
	}


	// Material.

	A_ModelMaterial material = {0};

	if (prim->material)
	{
		material = A_ModelResolveMaterial(ctx, arena, load, directory, prim->material);
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
		material.alpha_mode          = A_AlphaMode_Opaque;
		material.alpha_cutoff        = 0.5f;
	}


	// Push onto list.

	A_ModelLoadMesh *mesh = ArenaPushArray(arena, A_ModelLoadMesh, 1);
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
	
	load->total_vertex_bytes += vert_count * sizeof(A_ModelVertex);
	load->total_index_bytes  += idx_count  * sizeof(A_ModelIndex);

	if (is_skinned)
		load->total_skin_vertex_bytes += vert_count * sizeof(A_ModelSkinVertex);
}

internal void
A_ModelProcessNode(const A_Context *ctx,
					 Arena *arena,
					 A_ModelLoadData *load,
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
			mesh_transform = A_GltfTransformM4(A_GltfFloat16ToM4(world));
		}

		i32 skin_index = -1;

		if (is_skinned)
			skin_index = (i32)cgltf_skin_index(gltf, node->skin);

		for (u32 i = 0; i < node->mesh->primitives_count; i++)
		{
			const cgltf_primitive *prim = &node->mesh->primitives[i];
			A_ModelProcessPrimitive(ctx, arena, load, directory, prim, mesh_transform, node->skin, skin_index);
		}
	}

	for (u32 i = 0; i < node->children_count; i++)
	{
		A_ModelProcessNode(ctx, arena, load, directory, node->children[i], gltf);
	}
}

internal void
A_ModelLoadSkeleton(const A_Context *ctx,
					  Arena *arena,
					  A_ModelLoadData *load,
					  const cgltf_data *gltf,
					  u32 index)
{
	const cgltf_skin *skin = &gltf->skins[index];
	A_Skeleton *out = &load->skeletons[index];

	if (skin->name && skin->name[0])
		out->name = String8Clone(arena, String8FromCStr(skin->name));
	else if (skin->skeleton && skin->skeleton->name)
		out->name = String8Clone(arena, String8FromCStr(skin->skeleton->name));
	else
		out->name = String8Fmt(arena, "Unnamed Skeleton (%u)", index);
	
	out->joint_count = (u32)skin->joints_count;
	out->joints = ArenaPushArray(arena, A_Joint, out->joint_count);

	for (u32 i = 0; i < out->joint_count; i++)
	{
		A_Joint *j = &out->joints[i];

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

		j->bind_translation = A_GltfTransformTranslation(translation);
		j->bind_rotation    = A_GltfTransformQuat(rotation);
		j->bind_scale       = A_GltfTransformScale(scale);

		if (skin->inverse_bind_matrices)
		{
			f32 ibm[16] = {0};
			cgltf_accessor_read_float(skin->inverse_bind_matrices, i, ibm, 16);
			j->inverse_bind_matrix = A_GltfTransformM4(A_GltfFloat16ToM4(ibm));
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
		out->root_parent_world = A_GltfTransformM4(A_GltfFloat16ToM4(world));
	}
}

internal A_AnimPath
A_ModelAnimPathFromGltf(cgltf_animation_path_type t)
{
	switch (t)
	{
		case cgltf_animation_path_type_translation:  return A_AnimPath_Translate;
		case cgltf_animation_path_type_rotation:     return A_AnimPath_Rotation;
		case cgltf_animation_path_type_scale:        return A_AnimPath_Scale;
	}

	AssertTrue(false);

	return A_AnimPath_COUNT;
}

internal A_AnimInterp
A_ModelAnimInterpFromGltf(cgltf_interpolation_type t)
{
	switch (t)
	{
		case cgltf_interpolation_type_step:          return A_AnimInterp_Step;
		case cgltf_interpolation_type_linear:        return A_AnimInterp_Linear;
		case cgltf_interpolation_type_cubic_spline:  return A_AnimInterp_Cubic;
	}

	AssertTrue(false);

	return A_AnimInterp_COUNT;
}

/*
 * this function sucks dick holy shit
 */
internal void
A_ModelLoadClip(const A_Context *ctx,
				  Arena *arena,
				  A_ModelLoadData *load,
				  const cgltf_data *gltf,
				  u32 index)
{
	A_AnimClip *clip = &load->clips[index];

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
	clip->channels = ArenaPushArray(arena, A_AnimChannel, clip->channel_count);

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

		A_AnimChannel *ch = &clip->channels[curr];
			
		const cgltf_animation_sampler *anim_sampler = anim_ch->sampler;

		ch->target_skeleton = skeleton_idx;
		ch->target_joint    = (u32)joint_idx;

		ch->path            = A_ModelAnimPathFromGltf(anim_ch->target_path);
		ch->interp          = A_ModelAnimInterpFromGltf(anim_sampler->interpolation);

		b32 cubic = anim_sampler->interpolation == cgltf_interpolation_type_cubic_spline;

		if (cubic)
		{
			DebugLogW(ctx->log_channel,
					  "Clip (%.*s) channel uses cubic interpolation but we don't support that yet so falling back to linear.",
					  String8VArg(clip->name));

			ch->interp = A_AnimInterp_Linear;
		}

		u32 nkeys = (u32)anim_sampler->input->count;

		ch->key_count = nkeys;
		ch->keys = ArenaPushArray(arena, A_AnimKey, ch->key_count);

		for (u32 k = 0; k < ch->key_count; k++)
		{
			A_AnimKey *key = &ch->keys[k];

			f32 t = 0.f;
			cgltf_accessor_read_float(anim_sampler->input, k, &t, 1);
			key->timestamp_s = t;

			switch (ch->path)
			{
				case A_AnimPath_Translate:
				{
					f32 v[3] = { 0.f, 0.f, 0.f };
					cgltf_accessor_read_float(anim_sampler->output, k, v, 3);
					key->translation = A_GltfTransformTranslation(v3(v[0], v[1], v[2]));
				}
				break;
						
				case A_AnimPath_Rotation:
				{
					f32 v[4] = { 0.f, 0.f, 0.f, 1.f };
					cgltf_accessor_read_float(anim_sampler->output, k, v, 4);
					key->rotation = A_GltfTransformQuat(v4(v[0], v[1], v[2], v[3]));	
				}
				break;

				case A_AnimPath_Scale:
				{
					f32 v[3] = { 1.f, 1.f, 1.f };
					cgltf_accessor_read_float(anim_sampler->output, k, v, 3);
					key->scale = A_GltfTransformScale(v3(v[0], v[1], v[2]));
				}
				break;
			}
		}

		if (nkeys > 0 && ch->keys[nkeys - 1].timestamp_s > clip->duration_s)
			clip->duration_s = ch->keys[nkeys - 1].timestamp_s;
			
		curr++;
	}
}

internal A_SerializerPipelineData
A_ModelSerializerCpu(const A_Context *ctx, Arena *load_scope)
{
	ScratchArena scratch = ScratchBegin(NULL, 0);

	String8 file_path = A_ContextSystemFilePath(ctx, scratch.arena);

	A_ModelLoadData *load = ArenaPushArray(load_scope, A_ModelLoadData, 1);
	MemZeroStruct(load);

	A_SerializerPipelineData result = {0};
	result.data = load;

	cgltf_options options = {0};
	cgltf_data *gltf = NULL;

	if (cgltf_parse_file(&options, (const char *)file_path.str, &gltf) != cgltf_result_success)
	{
		DebugLogE(ctx->log_channel,
				  "Failed to parse glTF model: %.*s",
				  String8VArg(file_path));
		
		result.failed = true;
		goto end;
	}

	if (cgltf_load_buffers(&options, gltf, (const char *)file_path.str) != cgltf_result_success)
	{
		DebugLogE(ctx->log_channel,
				  "Failed to load buffers of glTF model: %.*s",
				  String8VArg(file_path));
		
		result.failed = true;
		goto end;
	}

	if (cgltf_validate(gltf) != cgltf_result_success)
	{
		DebugLogE(ctx->log_channel,
				  "Failed to validate glTF model: %.*s",
				  String8VArg(file_path));
		
		result.failed = true;
		goto end;
	}

	String8 directory = IO_PathGetFileDirectory(scratch.arena, ctx->metadata.path);

	const cgltf_scene *scene = gltf->scene;

	if (!scene && gltf->scenes_count > 0)
		scene = &gltf->scenes[0];

	if (!scene)
	{
		DebugLogE(ctx->log_channel, "Failed to create cgltf scene.");
		
		result.failed = true;
		goto end;
	}

	for (u32 i = 0; i < scene->nodes_count; i++)
		A_ModelProcessNode(ctx, load_scope, load, directory, scene->nodes[i], gltf);
	
	if (gltf->skins_count > 0)
	{
		load->skeleton_count = (u32)gltf->skins_count;
		load->skeletons = ArenaPushArray(load_scope, A_Skeleton, load->skeleton_count);

		for (u32 i = 0; i < load->skeleton_count; i++)
			A_ModelLoadSkeleton(ctx, load_scope, load, gltf, i);
		
		load->clip_count = (u32)gltf->animations_count;
		load->clips = ArenaPushArray(load_scope, A_AnimClip, load->clip_count);

		for (u32 i = 0; i < load->clip_count; i++)
			A_ModelLoadClip(ctx, load_scope, load, gltf, i);
	}
	
	result.stage_size = load->total_vertex_bytes + load->total_index_bytes + load->total_skin_vertex_bytes;
	result.failed = false;

	result.dependency_count = load->dep_count;

	if (load->dep_count > 0)
	{
		result.dependencies = ArenaPushArray(load_scope, A_Handle, load->dep_count);

		A_ModelLoadDep *dep = load->first_dep;

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
A_ModelSerializerAlloc(const A_Context *ctx,
						 A_SerializerPipelineData *data,
						 A_Asset *out,
						 Arena *arena)
{
	A_ModelLoadData *load = data->data;
	G_Device *device = ctx->assets->device;

	
	A_SubModel *sub_models = ArenaPushArray(arena, A_SubModel, load->mesh_count);
	
	A_ModelLoadMesh *src_mesh = load->first_mesh;

	for (i32 i = (i32)load->mesh_count - 1; i >= 0; i--, src_mesh = src_mesh->next)
	{
		A_SubModel *dst = &sub_models[i];

		dst->transform     = src_mesh->transform;

		dst->bounds_min    = src_mesh->bounds_min;
		dst->bounds_max    = src_mesh->bounds_max;

		dst->material      = src_mesh->material;

		dst->vertex_stride = sizeof(A_ModelVertex);
		dst->index_stride  = sizeof(A_ModelIndex);

		dst->vertex_count  = src_mesh->vertex_count;
		dst->index_count   = src_mesh->index_count;

		G_BufferAllocInfo vb_info = {0};
		vb_info.usage = VK_BUFFER_USAGE_2_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_2_TRANSFER_DST_BIT | VK_BUFFER_USAGE_2_TRANSFER_SRC_BIT;
		vb_info.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
		vb_info.size  = src_mesh->vertex_count * sizeof(A_ModelVertex);

		G_BufferAllocInfo ib_info = {0};
		ib_info.usage = VK_BUFFER_USAGE_2_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_2_TRANSFER_DST_BIT | VK_BUFFER_USAGE_2_TRANSFER_SRC_BIT;
		ib_info.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
		ib_info.size  = src_mesh->index_count * sizeof(A_ModelIndex);

		dst->vertex_buffer = G_DeviceBufferAlloc(device, &vb_info);
		dst->index_buffer  = G_DeviceBufferAlloc(device, &ib_info);

		if (src_mesh->skin_vertices)
		{
			G_BufferAllocInfo svb_info = {0};
			svb_info.usage = VK_BUFFER_USAGE_2_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_2_TRANSFER_DST_BIT | VK_BUFFER_USAGE_2_TRANSFER_SRC_BIT;
			svb_info.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
			svb_info.size  = src_mesh->vertex_count * sizeof(A_ModelSkinVertex);
			
			dst->skin_buffer = G_DeviceBufferAlloc(device, &svb_info);
		}
		else
		{
			dst->skin_buffer = G_BufferKeyNull();
		}
		
		dst->skin_index = src_mesh->skin_index;
	}

	
	A_Skeleton *skeletons = NULL;

	if (load->skeleton_count > 0)
		skeletons = ArenaPushArray(arena, A_Skeleton, load->skeleton_count);

	for (u32 i = 0; i < load->skeleton_count; i++)
	{
		A_Skeleton *src = &load->skeletons[i];
		A_Skeleton *dst = &skeletons[i];

		dst->name = String8Clone(arena, src->name);
		
		dst->joint_count = src->joint_count;
		dst->joints = ArenaPushArray(arena, A_Joint, src->joint_count);

		dst->root_parent_world = src->root_parent_world;

		for (u32 j = 0; j < dst->joint_count; j++)
		{
			A_Joint *src_j = &src->joints[j];
			A_Joint *dst_j = &dst->joints[j];
			
			dst_j->name                = String8Clone(arena, src_j->name);
			
			dst_j->parent              = src_j->parent;

			dst_j->bind_translation    = src_j->bind_translation;
			dst_j->bind_rotation       = src_j->bind_rotation;
			dst_j->bind_scale          = src_j->bind_scale;

			dst_j->inverse_bind_matrix = src_j->inverse_bind_matrix;
		}
	}

	
	A_AnimClip *clips = NULL;

	if (load->clip_count)
		clips = ArenaPushArray(arena, A_AnimClip, load->clip_count);

	for (u32 i = 0; i < load->clip_count; i++)
	{
		A_AnimClip *src = &load->clips[i];
		A_AnimClip *dst = &clips[i];

		dst->name = String8Clone(arena, src->name);
		
		dst->duration_s = src->duration_s;

		dst->channel_count = src->channel_count;
		dst->channels = ArenaPushArray(arena, A_AnimChannel, src->channel_count);

		for (u32 j = 0; j < dst->channel_count; j++)
		{
			A_AnimChannel *src_ch = &src->channels[j];
			A_AnimChannel *dst_ch = &dst->channels[j];

			dst_ch->target_skeleton = src_ch->target_skeleton;
			dst_ch->target_joint    = src_ch->target_joint;

			dst_ch->path            = src_ch->path;
			dst_ch->interp          = src_ch->interp;
			
			dst_ch->key_count       = src_ch->key_count;
			dst_ch->keys            = ArenaPushArray(arena, A_AnimKey, dst_ch->key_count);

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
A_ModelSerializerReload(const A_Context *ctx,
						  A_SerializerPipelineData *data,
						  A_Asset *existing)
{
	DebugLogW(ctx->log_channel, "Reloading not implemented yet.");
}

internal void
A_ModelSerializerGpu(const A_Context *ctx,
					   A_SerializerPipelineData *data,
					   A_Asset *asset,
					   G_CmdBuffer *cmd,
					   G_BufferKey stage, u64 stage_base)
{
	A_ModelLoadData *load = data->data;
	G_Device *device = ctx->assets->device;

	u64 offset = stage_base;

	ScratchArena scratch = ScratchBegin(NULL, 0);
	
	A_ModelLoadMesh **load_meshes = ArenaPushArray(scratch.arena, A_ModelLoadMesh *, load->mesh_count);

	A_ModelLoadMesh *src_mesh = load->first_mesh;

	for (i32 i = (i32)load->mesh_count - 1; i >= 0; i--, src_mesh = src_mesh->next)
	{
		load_meshes[i] = src_mesh;
	}

	for (u32 i = 0; i < asset->model.sub_model_count; i++)
	{
		A_ModelLoadMesh *src = load_meshes[i];
		A_SubModel      *dst = &asset->model.sub_models[i];


		// Vertices.
		
		u64 vb_size = src->vertex_count * sizeof(A_ModelVertex);
		
		G_DeviceBufferWrite(device, stage, src->vertices, vb_size,  offset);
		
		G_BufferCopy vb_copy = {0};
		vb_copy.src_offset = offset;
		vb_copy.size = vb_size;
		
		G_CmdCopyBufferToBuffer(cmd, stage, dst->vertex_buffer, 1, &vb_copy);

		offset += vb_size;


		// Indices.
		
		u64 ib_size = src->index_count * sizeof(A_ModelIndex);

		G_DeviceBufferWrite(device, stage, src->indices, ib_size,  offset);

		G_BufferCopy ib_copy = {0};
		ib_copy.src_offset = offset;
		ib_copy.size = ib_size;

		G_CmdCopyBufferToBuffer(cmd, stage, dst->index_buffer,  1, &ib_copy);
		
		offset += ib_size;


		// Skinning.
		
		if (src->skin_vertices)
		{
			u64 svb_size = src->vertex_count * sizeof(A_ModelSkinVertex);
			
			G_DeviceBufferWrite(device, stage, src->skin_vertices, svb_size, offset);

			G_BufferCopy svb_copy = {0};
			svb_copy.src_offset = offset;
			svb_copy.size = svb_size;

			G_CmdCopyBufferToBuffer(cmd, stage, dst->skin_buffer, 1, &svb_copy);

			offset += svb_size;
		}
	}

	ScratchRelease(&scratch);
}

internal void
A_ModelSerializerDispose(A_Asset *asset, A_Registry *assets)
{
	G_Device *device = assets->device;

	for (u32 i = 0; i < asset->model.sub_model_count; i++)
	{
		A_SubModel *sub_model = &asset->model.sub_models[i];
		
		G_DeviceBufferDestroy(device, sub_model->vertex_buffer);
		G_DeviceBufferDestroy(device, sub_model->index_buffer);

		if (!G_BufferKeyIsNull(sub_model->skin_buffer))
			G_DeviceBufferDestroy(device, sub_model->skin_buffer);
	}
}

internal A_Serializer
A_GetModelSerializer(void)
{
	static A_Serializer model_serializer = {
		.Cpu     = A_ModelSerializerCpu,
		.Alloc   = A_ModelSerializerAlloc,
		.Reload  = A_ModelSerializerReload,
		.Gpu     = A_ModelSerializerGpu,
		.End     = NULL,
		.Dispose = A_ModelSerializerDispose,
	};

	return model_serializer;
}
