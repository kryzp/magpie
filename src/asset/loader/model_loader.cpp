#include "asset/asset_loader.h"
#include "asset/asset_system.h"
#include "graphics/device.h"
#include "graphics/cmd_buffer.h"
#include "io/path.h"

#include <gltf/cgltf.h>

#include <cstdio>

/*
 * magpie is right-handed z-up.
 * glTF is right-handed y-up.
 * therefore we gotta translate between 'em.
 */
static Mat4 GltfBasis()
{
	Mat4 m;
	m.c[0] = Vec4(1.f, 0.f, 0.f, 0.f);
	m.c[1] = Vec4(0.f, 0.f, 1.f, 0.f);
	m.c[2] = Vec4(0.f, -1.f, 0.f, 0.f);
	m.c[3] = Vec4(0.f, 0.f, 0.f, 1.f);
	return m;
}

static Mat4 GltfBasisInv()
{
	Mat4 m;
	m.c[0] = Vec4(1.f, 0.f, 0.f, 0.f);
	m.c[1] = Vec4(0.f, 0.f, -1.f, 0.f);
	m.c[2] = Vec4(0.f, 1.f, 0.f, 0.f);
	m.c[3] = Vec4(0.f, 0.f, 0.f, 1.f);
	return m;
}

static Mat4 GltfFloat16ToMat4(const cgltf_float m[16])
{
	Mat4 result;

	for (int i = 0; i < 16; i++)
		(&result.m00)[i] = m[i];

	return result;
}

static Vec3 GltfTransformTranslation(const Vec3 &v)
{
	return Vec3(v.x, -v.z, v.y);
}

static Quat GltfTransformQuat(const Quat &q)
{
	return Quat(q.x, -q.z, q.y, q.w);
}

static Vec3 GltfTransformScale(const Vec3 &s)
{
	return Vec3(s.x, s.z, s.y);
}

static Mat4 GltfTransformMat4(const Mat4 &m)
{
	return GltfBasis() * (m * GltfBasisInv());
}

static String FormatName(const char *fmt, unsigned a, unsigned b = 0)
{
	char buf[128] = {};
	snprintf(buf, sizeof(buf), fmt, a, b);
	return buf;
}

struct ModelLoadSubModel {
	Mat4 transform;

	Vec3 bounds_min;
	Vec3 bounds_max;

	ast::ModelMaterial material;

	Vector<ast::ModelVertex> vertices;
	Vector<u32> indices;

	Vector<ast::ModelSkinVertex> skin_vertices;
	int skin_index = -1;
};

struct ModelLoadData : ast::LoadData {
	Vector<ast::Handle> dependencies;

	Vector<ModelLoadSubModel> submodels;

	Vector<ast::Skeleton> skeletons;
	Vector<ast::AnimClip> clips;

	u64 total_vertex_bytes = 0;
	u64 total_index_bytes = 0;
	u64 total_skin_vertex_bytes = 0;
};

static ast::Handle TryFetchTexture(
	const ast::LoaderContext &ctx,
	ModelLoadData *load,
	const String &directory,
	const cgltf_texture_view *view,
	bool srgb
)
{
	if (!view->texture)
		return ast::Handle();

	const cgltf_image *image = view->texture->image;

	if (view->texture->has_basisu && view->texture->basisu_image)
		image = view->texture->basisu_image;

	if (!image)
		return ast::Handle();

	// TODO: handle data: URIs (base64 embedded)
	//       and bufferview-backed images (.glb).
	if (!image->uri) {
		DebugLogB(ctx.log_channel, "URI's not supported yet.");
		return ast::Handle();
	}

	if (strncmp(image->uri, "data:", 5) == 0)
		return ast::Handle();

	String uri = image->uri;
	uri.resize(cgltf_decode_uri(uri.data()));
	String full_path = directory + uri;

	ast::Handle handle = ast::AssetSystem::Get()->HandleFromFilePath(full_path);

	DebugLogAssert(ctx.log_channel, handle.type == ast::AssetType_Texture, "Model dependency that isn't a texture asset!");

	if (srgb)
		ast::AssetSystem::Get()->MarkTextureAsSRGB(handle);

	load->dependencies.push_back(handle);

	return handle;
}

static ast::ModelMaterial ResolveMaterial(
	const ast::LoaderContext &ctx,
	ModelLoadData *load,
	const String &directory,
	const cgltf_material *gltf_mat,
	unsigned index
)
{
	ast::ModelMaterial mat;

	if (gltf_mat->name && gltf_mat->name[0])
		mat.name = gltf_mat->name;
	else
		mat.name = FormatName("Unnamed Material (%u)", index);

	if (gltf_mat->has_pbr_specular_glossiness) {

		// this is essentially a legacy form of representing
		// materials but some models still go through with it,
		// like the amazon lumberyard bistro.

		// SPECULAR-GLOSSINESS PBR.

		const cgltf_pbr_specular_glossiness *sg = &gltf_mat->pbr_specular_glossiness;

		mat.albedo_texture = TryFetchTexture(ctx, load, directory, &sg->diffuse_texture, true);
		mat.albedo_factor = Vec4(sg->diffuse_factor[0], sg->diffuse_factor[1], sg->diffuse_factor[2], sg->diffuse_factor[3]);

		mat.metallic_factor = 0.f;
		mat.roughness_factor = 1.f - sg->glossiness_factor;

		mat.specular_colour_texture = TryFetchTexture(ctx, load, directory, &sg->specular_glossiness_texture, true);
		mat.specular_colour_factor = Vec3(sg->specular_factor[0], sg->specular_factor[1], sg->specular_factor[2]);
		mat.specular_factor = 1.f;

		mat.ior = 1000.f;

		mat.gloss_in_specular_alpha = true;

	} else if (gltf_mat->has_pbr_metallic_roughness) {

		// METALLIC-ROUGHNESS OPAQUE PBR.

		const cgltf_pbr_metallic_roughness *mr = &gltf_mat->pbr_metallic_roughness;

		mat.albedo_texture = TryFetchTexture(ctx, load, directory, &mr->base_color_texture, true);

		mat.metallic_roughness_texture = TryFetchTexture(ctx, load, directory, &mr->metallic_roughness_texture, false);

		mat.albedo_factor = Vec4(
			mr->base_color_factor[0],
			mr->base_color_factor[1],
			mr->base_color_factor[2],
			mr->base_color_factor[3]
		);

		mat.metallic_factor = mr->metallic_factor;
		mat.roughness_factor = mr->roughness_factor;
	}

	// NORMALS.
	mat.normal_texture = TryFetchTexture(ctx, load, directory, &gltf_mat->normal_texture, false);
	mat.normal_scale = gltf_mat->normal_texture.scale;

	// OCCLUSION.
	mat.occlusion_texture = TryFetchTexture(ctx, load, directory, &gltf_mat->occlusion_texture, false);
	mat.occlusion_intensity = gltf_mat->occlusion_texture.scale;

	// EMISSIVE.
	mat.emissive_texture = TryFetchTexture(ctx, load, directory, &gltf_mat->emissive_texture, true);
	mat.emissive_factor = Vec3(
		gltf_mat->emissive_factor[0],
		gltf_mat->emissive_factor[1],
		gltf_mat->emissive_factor[2]
	);

	// EMISSIVE STRENGTH.
	if (gltf_mat->has_emissive_strength)
		mat.emissive_intensity = gltf_mat->emissive_strength.emissive_strength;

	// INDEX OF REFRACTION.
	if (gltf_mat->has_ior)
		mat.ior = gltf_mat->ior.ior;

	// TRANSMISSION.
	if (gltf_mat->has_transmission) {
		mat.transmission_texture = TryFetchTexture(ctx, load, directory, &gltf_mat->transmission.transmission_texture, false);
		mat.transmission_factor = gltf_mat->transmission.transmission_factor;
	}

	// VOLUME.
	if (gltf_mat->has_volume) {
		mat.thickness_texture = TryFetchTexture(ctx, load, directory, &gltf_mat->volume.thickness_texture, false);
		mat.thickness_factor = gltf_mat->volume.thickness_factor;

		mat.attenuation_colour = Vec3(
			gltf_mat->volume.attenuation_color[0],
			gltf_mat->volume.attenuation_color[1],
			gltf_mat->volume.attenuation_color[2]
		);

		mat.attenuation_distance = gltf_mat->volume.attenuation_distance;
	}

	// SPECULAR.
	if (gltf_mat->has_specular) {
		mat.specular_texture = TryFetchTexture(ctx, load, directory, &gltf_mat->specular.specular_texture, false);
		mat.specular_colour_texture = TryFetchTexture(ctx, load, directory, &gltf_mat->specular.specular_color_texture, true);

		mat.specular_factor = gltf_mat->specular.specular_factor;
		mat.specular_colour_factor = Vec3(
			gltf_mat->specular.specular_color_factor[0],
			gltf_mat->specular.specular_color_factor[1],
			gltf_mat->specular.specular_color_factor[2]
		);
	}

	// CLEARCOAT.
	if (gltf_mat->has_clearcoat) {
		mat.clearcoat_texture = TryFetchTexture(ctx, load, directory, &gltf_mat->clearcoat.clearcoat_texture, false);
		mat.clearcoat_roughness_texture = TryFetchTexture(ctx, load, directory, &gltf_mat->clearcoat.clearcoat_roughness_texture, false);

		mat.clearcoat_factor = gltf_mat->clearcoat.clearcoat_factor;
		mat.clearcoat_roughness_factor = gltf_mat->clearcoat.clearcoat_roughness_factor;
	}

	// SHEEN.
	if (gltf_mat->has_sheen) {
		mat.sheen_colour_texture = TryFetchTexture(ctx, load, directory, &gltf_mat->sheen.sheen_color_texture, true);
		mat.sheen_roughness_texture = TryFetchTexture(ctx, load, directory, &gltf_mat->sheen.sheen_roughness_texture, false);

		mat.sheen_colour_factor = Vec3(
			gltf_mat->sheen.sheen_color_factor[0],
			gltf_mat->sheen.sheen_color_factor[1],
			gltf_mat->sheen.sheen_color_factor[2]
		);

		mat.sheen_roughness_factor = gltf_mat->sheen.sheen_roughness_factor;
	}

	// IRIDESCENCE.
	if (gltf_mat->has_iridescence) {
		mat.iridescence_texture = TryFetchTexture(ctx, load, directory, &gltf_mat->iridescence.iridescence_texture, false);
		mat.iridescence_thickness_texture = TryFetchTexture(ctx, load, directory, &gltf_mat->iridescence.iridescence_thickness_texture, false);

		mat.iridescence_factor = gltf_mat->iridescence.iridescence_factor;
		mat.iridescence_ior = gltf_mat->iridescence.iridescence_ior;
		mat.iridescence_thickness_min_nanometers = gltf_mat->iridescence.iridescence_thickness_min;
		mat.iridescence_thickness_max_nanometers = gltf_mat->iridescence.iridescence_thickness_max;
	}

	// DOUBLE SIDED.
	mat.double_sided = !!gltf_mat->double_sided;

	// UNLIT.
	mat.unlit = !!gltf_mat->unlit;

	// ALPHA.
	switch (gltf_mat->alpha_mode) {
		case cgltf_alpha_mode_opaque:
			mat.alpha_mode = ast::AlphaMode_Opaque;
			break;
		case cgltf_alpha_mode_mask:
			mat.alpha_mode = ast::AlphaMode_Mask;
			break;
		case cgltf_alpha_mode_blend:
			mat.alpha_mode = ast::AlphaMode_Blend;
			break;
		default:
			mat.alpha_mode = ast::AlphaMode_Opaque;
			break;
	}

	mat.alpha_cutoff = gltf_mat->alpha_cutoff;

	DebugLogT(ctx.log_channel, "Loaded Material: %s", mat.name.c_str());

	return mat;
}

static void ProcessPrimitive(
	const ast::LoaderContext &ctx,
	ModelLoadData *load,
	const String &directory,
	const cgltf_primitive *prim,
	const Mat4 &world_transform,
	const cgltf_skin *skin,
	int skin_index,
	unsigned index
)
{
	// TODO: support more topologies
	DebugLogAssert(
		ctx.log_channel,
		prim->type == cgltf_primitive_type_triangles,
		"Only support triangle-primitive models currently!"
	);

	const cgltf_accessor *positions = nullptr;
	const cgltf_accessor *normals = nullptr;
	const cgltf_accessor *tangents = nullptr;
	const cgltf_accessor *texcoords = nullptr;
	const cgltf_accessor *colours = nullptr;
	const cgltf_accessor *joints = nullptr;
	const cgltf_accessor *weights = nullptr;

	for (int i = 0; i < prim->attributes_count; i++) {
		const cgltf_attribute *attr = &prim->attributes[i];

		switch (attr->type) {
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

	if (!positions) {
		DebugLogW(ctx.log_channel, "Node has no positions??");
		return;
	}

	// Skinning.
	bool is_skinned = (
		(skin != nullptr) &&
		(joints != nullptr) &&
		(weights != nullptr)
	);

	if ((skin != nullptr) && (joints == nullptr || weights == nullptr)) {
		DebugLogW(
			ctx.log_channel,
			"Node has skin but missing joints and/or weights."
		);
	}

	ModelLoadSubModel submodel;

	// Vertices.
	unsigned vert_count = positions->count;
	submodel.vertices.resize(vert_count);

	if (is_skinned) {
		submodel.skin_vertices.resize(vert_count);

		DebugLogAssert(
			ctx.log_channel,
			joints->count == positions->count &&
			weights->count == positions->count,
			"Joint / Weight count is in mismatch with position count in node."
		);
	}

	Vec3 bmin = Vec3(maths::MAX_FLOAT, maths::MAX_FLOAT, maths::MAX_FLOAT);
	Vec3 bmax = Vec3(-maths::MAX_FLOAT, -maths::MAX_FLOAT, -maths::MAX_FLOAT);

	for (int i = 0; i < vert_count; i++) {
		ast::ModelVertex *v = &submodel.vertices[i];

		float pos[3] = {};
		cgltf_accessor_read_float(positions, i, pos, 3);

		v->position = GltfTransformTranslation(Vec3(pos[0], pos[1], pos[2]));

		bmin = bmin.MinOf(v->position);
		bmax = bmax.MaxOf(v->position);

		if (texcoords) {
			float uv[2] = {};
			cgltf_accessor_read_float(texcoords, i, uv, 2);
			v->texcoord = Vec2(uv[0], uv[1]);
		} else {
			v->texcoord = Vec2(0.f);
		}

		if (colours) {
			float col[4] = { 1.f, 1.f, 1.f, 1.f };

			unsigned comps = static_cast<unsigned>(cgltf_num_components(colours->type));

			if (comps > 4)
				comps = 4;

			cgltf_accessor_read_float(colours, i, col, comps);

			v->colour = Vec3(col[0], col[1], col[2]);
		} else {
			v->colour = Vec3(1.f);
		}

		if (normals) {
			float n[3] = {};
			cgltf_accessor_read_float(normals, i, n, 3);

			v->normal = GltfTransformTranslation(Vec3(n[0], n[1], n[2]));
		} else {
			// TODO: derive smooth normals if missing.
			//       --> shouldn't be a problem as glTF assets usually have them... i think
			v->normal = Vec3(0.f);
		}

		if (tangents) {
			// glTF tangents are vec4: xyz = tangent, w = bitangent sign.
			float t[4] = {};
			cgltf_accessor_read_float(tangents, i, t, 4);

			Vec3 N = v->normal;
			Vec3 T = GltfTransformTranslation(Vec3(t[0], t[1], t[2]));

			float sign = t[3];

			v->tangent = T;
			v->bitangent = Vec3(
				(N.y * T.z - N.z * T.y) * sign,
				(N.z * T.x - N.x * T.z) * sign,
				(N.x * T.y - N.y * T.x) * sign
			);
		} else {
			// TODO: integrate using something called "mikktspace" for tangents.
			v->tangent = Vec3(0.f);
			v->bitangent = Vec3(0.f);
		}

		if (is_skinned) {
			ast::ModelSkinVertex *sv = &submodel.skin_vertices[i];

			cgltf_uint j[4] = {};
			cgltf_accessor_read_uint(joints, i, j, 4);

			DebugLogAssert(
				ctx.log_channel,
				j[0] < skin->joints_count &&
				j[1] < skin->joints_count &&
				j[2] < skin->joints_count &&
				j[3] < skin->joints_count,
				"Joint index is out of range."
			);

			sv->joints[0] = j[0];
			sv->joints[1] = j[1];
			sv->joints[2] = j[2];
			sv->joints[3] = j[3];

			float w[4] = {};
			cgltf_accessor_read_float(weights, i, w, 4);

			// renormalize 'cuz weights don't necessarily always
			// have to add up to 1 for whatever fucking reason.
			float wsum = w[0] + w[1] + w[2] + w[3];

			if (wsum > 0.f) {
				float inv = 1.f / wsum;

				w[0] *= inv;
				w[1] *= inv;
				w[2] *= inv;
				w[3] *= inv;
			} else {
				w[0] = 1.f; // zero influence so just bind to joint 0
			}

			sv->weights[0] = w[0];
			sv->weights[1] = w[1];
			sv->weights[2] = w[2];
			sv->weights[3] = w[3];
		}
	}

	// Indices.
	if (prim->indices) {
		unsigned idx_count = prim->indices->count;

		submodel.indices.resize(idx_count);

		for (int i = 0; i < idx_count; i++) {
			cgltf_uint idx = 0;
			cgltf_accessor_read_uint(prim->indices, i, &idx, 1);

			submodel.indices[i] = idx;
		}
	} else {
		// Non-indexed primitive: emit sequential indices.
		// todo: is this up to spec?

		submodel.indices.resize(vert_count);

		for (int i = 0; i < vert_count; i++)
			submodel.indices[i] = i;
	}

	// Material.
	if (prim->material)
		submodel.material = ResolveMaterial(ctx, load, directory, prim->material, index);
	else
		submodel.material = ast::ModelMaterial();

	submodel.transform = world_transform;
	submodel.bounds_min = bmin;
	submodel.bounds_max = bmax;
	submodel.skin_index = skin_index;

	load->total_vertex_bytes += vert_count * sizeof(ast::ModelVertex);
	load->total_index_bytes += submodel.indices.size() * sizeof(u32);

	if (is_skinned)
		load->total_skin_vertex_bytes += vert_count * sizeof(ast::ModelSkinVertex);

	load->submodels.push_back(std::move(submodel));
}

static void ProcessNode(
	const ast::LoaderContext &ctx,
	ModelLoadData *load,
	const String &directory,
	const cgltf_node *node,
	const cgltf_data *gltf
)
{
	if (node->mesh) {
		bool is_skinned = node->skin != nullptr;

		Mat4 mesh_transform;

		if (is_skinned) {
			mesh_transform = Mat4::Identity(); // glTf spec, node transform ignored, entirely based on joint.
		} else {
			cgltf_float world[16] = {};
			cgltf_node_transform_world(node, world);

			mesh_transform = GltfTransformMat4(GltfFloat16ToMat4(world));
		}

		int skin_index = -1;

		if (is_skinned)
			skin_index = static_cast<int>(cgltf_skin_index(gltf, node->skin));

		for (int i = 0; i < node->mesh->primitives_count; i++) {
			const cgltf_primitive *prim = &node->mesh->primitives[i];
			ProcessPrimitive(ctx, load, directory, prim, mesh_transform, node->skin, skin_index, i);
		}
	}

	for (int i = 0; i < node->children_count; i++)
		ProcessNode(ctx, load, directory, node->children[i], gltf);
}

static void LoadSkeleton(
	const ast::LoaderContext &ctx,
	ModelLoadData *load,
	const cgltf_data *gltf,
	unsigned index
)
{
	const cgltf_skin *skin = &gltf->skins[index];
	ast::Skeleton *out = &load->skeletons[index];

	if (skin->name && skin->name[0])
		out->name = skin->name;
	else if (skin->skeleton && skin->skeleton->name)
		out->name = skin->skeleton->name;
	else
		out->name = FormatName("Unnamed Skeleton (%u)", index);

	out->joints.resize(skin->joints_count);

	for (int i = 0; i < out->joints.size(); i++) {
		ast::Joint *j = &out->joints[i];
		const cgltf_node *node = skin->joints[i];

		if (node->name)
			j->name = node->name;
		else
			j->name = FormatName("Unnamed Joint (%u:%u)", index, i);

		j->parent = -1;

		if (node->parent) {
			for (int k = 0; k < out->joints.size(); k++) {
				if (skin->joints[k] != node->parent)
					continue;

				j->parent = k;
				break;
			}
		}

		Vec3 translation = Vec3(0.f, 0.f, 0.f);
		Quat rotation = Quat(0.f, 0.f, 0.f, 1.f);
		Vec3 scale = Vec3(1.f, 1.f, 1.f);

		if (node->has_matrix) {
			DebugLogW(ctx.log_channel, "Joint (%u:%u) stores a baked matrix but I'm not bothering to decompose the matrix yet so we're just gonna assume identity and act like everything's fine :thumbsup:", index, i);
		} else {
			if (node->has_translation) {
				translation = Vec3(
					node->translation[0],
					node->translation[1],
					node->translation[2]
				);
			}

			if (node->has_rotation) {
				rotation = Quat(
					node->rotation[0],
					node->rotation[1],
					node->rotation[2],
					node->rotation[3]
				);
			}

			if (node->has_scale) {
				scale = Vec3(
					node->scale[0],
					node->scale[1],
					node->scale[2]
				);
			}
		}

		j->bind_translation = GltfTransformTranslation(translation);
		j->bind_rotation = GltfTransformQuat(rotation);
		j->bind_scale = GltfTransformScale(scale);

		if (skin->inverse_bind_matrices) {
			float ibm[16] = {};
			cgltf_accessor_read_float(skin->inverse_bind_matrices, i, ibm, 16);

			j->inverse_bind_matrix = GltfTransformMat4(GltfFloat16ToMat4(ibm));
		} else {
			j->inverse_bind_matrix = Mat4::Identity();
		}
	}

	out->root_parent_world = Mat4::Identity();

	const cgltf_node *top_joint_n = nullptr;

	for (int i = 0; i < skin->joints_count; i++) {
		const cgltf_node *n = skin->joints[i];

		bool is_parent = false;

		if (n->parent) {
			for (int k = 0; k < skin->joints_count; k++) {
				if (skin->joints[k] != n->parent)
					continue;

				is_parent = true;
				break;
			}
		}

		if (!is_parent) {
			top_joint_n = n;
			break;
		}
	}

	if (top_joint_n && top_joint_n->parent) {
		cgltf_float world[16] = {};
		cgltf_node_transform_world(top_joint_n->parent, world);

		out->root_parent_world = GltfTransformMat4(GltfFloat16ToMat4(world));
	}
}

static ast::AnimPath AnimPathFromGltf(cgltf_animation_path_type t)
{
	switch (t) {
		case cgltf_animation_path_type_translation:
			return ast::AnimPath_Translate;
		case cgltf_animation_path_type_rotation:
			return ast::AnimPath_Rotation;
		case cgltf_animation_path_type_scale:
			return ast::AnimPath_Scale;
		default:
			break;
	}

	AssertTrue(false);

	return ast::AnimPath_MaxEnum;
}

static ast::AnimInterp AnimInterpFromGltf(cgltf_interpolation_type t)
{
	switch (t) {
		case cgltf_interpolation_type_step:
			return ast::AnimInterp_Step;
		case cgltf_interpolation_type_linear:
			return ast::AnimInterp_Linear;
		case cgltf_interpolation_type_cubic_spline:
			return ast::AnimInterp_Cubic;
		default:
			break;
	}

	AssertTrue(false);

	return ast::AnimInterp_MaxEnum;
}

static void FindAnimTarget(const cgltf_data *gltf, const cgltf_node *target, int *out_skeleton, int *out_joint)
{
	*out_skeleton = -1;
	*out_joint = -1;

	for (int ii = 0; ii < gltf->skins_count; ii++) {
		if (*out_joint >= 0)
			break;

		const cgltf_skin *s = &gltf->skins[ii];

		for (int jj = 0; jj < s->joints_count; jj++) {
			if (s->joints[jj] == target) {
				*out_joint = jj;
				*out_skeleton = ii;
				break;
			}
		}
	}
}

static void LoadClip(
	const ast::LoaderContext &ctx,
	ModelLoadData *load,
	const cgltf_data *gltf,
	unsigned index
)
{
	ast::AnimClip *clip = &load->clips[index];
	const cgltf_animation *anim = &gltf->animations[index];

	if (anim->name)
		clip->name = anim->name;
	else
		clip->name = FormatName("Animation Clip (%u)", index);

	clip->duration_s = 0.f;

	for (int j = 0; j < anim->channels_count; j++) {
		const cgltf_animation_channel *anim_ch = &anim->channels[j];

		if (!anim_ch->target_node || !anim_ch->sampler)
			continue;

		if (anim_ch->target_path == cgltf_animation_path_type_invalid)
			continue;

		if (anim_ch->target_path == cgltf_animation_path_type_weights)
			continue; // morph targets

		int skeleton_idx = -1;
		int joint_idx = -1;

		FindAnimTarget(gltf, anim_ch->target_node, &skeleton_idx, &joint_idx);

		if (joint_idx < 0)
			continue;

		ast::AnimChannel ch;

		const cgltf_animation_sampler *anim_sampler = anim_ch->sampler;

		ch.target_skeleton = skeleton_idx;
		ch.target_joint = joint_idx;
		ch.path = AnimPathFromGltf(anim_ch->target_path);
		ch.interp = AnimInterpFromGltf(anim_sampler->interpolation);

		bool cubic = anim_sampler->interpolation == cgltf_interpolation_type_cubic_spline;

		if (cubic) {
			DebugLogW(
				ctx.log_channel,
				"Clip (%s) channel uses cubic interpolation but we lowkey don't support that yet so we're just gonna fall back to linear and hope for the best... :/",
				clip->name.c_str()
			);

			ch.interp = ast::AnimInterp_Linear;
		}

		unsigned nkeys = anim_sampler->input->count;

		ch.keys.resize(nkeys);

		for (int k = 0; k < nkeys; k++) {
			ast::AnimKey *key = &ch.keys[k];

			float t = 0.f;
			cgltf_accessor_read_float(anim_sampler->input, k, &t, 1);

			key->timestamp_s = t;

			switch (ch.path) {
				case ast::AnimPath_Translate: {
					float v[3] = { 0.f, 0.f, 0.f };
					cgltf_accessor_read_float(anim_sampler->output, k, v, 3);
					key->translation = GltfTransformTranslation(Vec3(v[0], v[1], v[2]));
				} break;

				case ast::AnimPath_Rotation: {
					float v[4] = { 0.f, 0.f, 0.f, 1.f };
					cgltf_accessor_read_float(anim_sampler->output, k, v, 4);
					key->rotation = GltfTransformQuat(Quat(v[0], v[1], v[2], v[3]));
				} break;

				case ast::AnimPath_Scale: {
					float v[3] = { 1.f, 1.f, 1.f };
					cgltf_accessor_read_float(anim_sampler->output, k, v, 3);
					key->scale = GltfTransformScale(Vec3(v[0], v[1], v[2]));
				} break;

				default:
					break;
			}
		}

		if (nkeys > 0 && ch.keys[nkeys - 1].timestamp_s > clip->duration_s)
			clip->duration_s = ch.keys[nkeys - 1].timestamp_s;

		clip->channels.push_back(std::move(ch));
	}
}

const Vector<String> &ast::ModelLoader::GetFileExtensions() const
{
	static const Vector<String> file_extensions = {
		"gltf"
	};

	return file_extensions;
}

ast::LoadResult ast::ModelLoader::Load(const LoaderContext &ctx)
{
	String file_path = AssetSystem::Get()->GetSystemFilePath(ctx.metadata.path);

	ModelLoadData *load = new ModelLoadData();

	LoadResult result;
	result.user_data = load;

	cgltf_options options = {};
	cgltf_data *gltf = nullptr;

	if (cgltf_parse_file(&options, file_path.c_str(), &gltf) != cgltf_result_success) {
		DebugLogE(ctx.log_channel, "Failed to parse glTF model: %s", file_path.c_str());

		result.failed = true;
		return result;
	}

	if (cgltf_load_buffers(&options, gltf, file_path.c_str()) != cgltf_result_success) {
		DebugLogE(ctx.log_channel, "Failed to load buffers of glTF model: %s", file_path.c_str());

		cgltf_free(gltf);

		result.failed = true;
		return result;
	}

	if (cgltf_validate(gltf) != cgltf_result_success) {
		DebugLogE(ctx.log_channel, "Failed to validate glTF model: %s", file_path.c_str());

		cgltf_free(gltf);

		result.failed = true;
		return result;
	}

	String directory = path::GetFileDirectory(ctx.metadata.path);

	const cgltf_scene *scene = gltf->scene;

	if (!scene && gltf->scenes_count > 0)
		scene = &gltf->scenes[0];

	if (!scene) {
		DebugLogE(ctx.log_channel, "Failed to create cgltf scene.");

		cgltf_free(gltf);

		result.failed = true;
		return result;
	}

	for (int i = 0; i < scene->nodes_count; i++)
		ProcessNode(ctx, load, directory, scene->nodes[i], gltf);

	if (gltf->skins_count > 0) {
		load->skeletons.resize(gltf->skins_count);

		for (int i = 0; i < load->skeletons.size(); i++)
			LoadSkeleton(ctx, load, gltf, i);
	}

	if (gltf->animations_count > 0) {
		load->clips.resize(gltf->animations_count);

		for (int i = 0; i < load->clips.size(); i++)
			LoadClip(ctx, load, gltf, i);
	}

	result.stage_size = load->total_skin_vertex_bytes;
	result.failed = false;
	result.dependencies = load->dependencies;

	cgltf_free(gltf);

	return result;
}

void ast::ModelLoader::Alloc(const LoaderContext &ctx, LoadResult *result, Asset *asset)
{
	ModelLoadData *load = static_cast<ModelLoadData *>(result->user_data);

	gfx::Device *device = gfx::Device::Get();

	asset->model.sub_models.resize(load->submodels.size());

	for (int i = 0; i < load->submodels.size(); i++) {
		const ModelLoadSubModel *src = &load->submodels[i];
		SubModel *dst = &asset->model.sub_models[i];

		dst->transform = src->transform;
		dst->bounds_min = src->bounds_min;
		dst->bounds_max = src->bounds_max;
		dst->material = src->material;

		dst->vertices = src->vertices;
		dst->vertex_stride = sizeof(ModelVertex);

		dst->indices = src->indices;
		dst->index_type = gfx::IndexType_U32; // TODO: make this variable based on model??

		if (!src->skin_vertices.empty()) {
			gfx::BufferAllocInfo svb_info;
			svb_info.usage = VK_BUFFER_USAGE_2_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_2_TRANSFER_DST_BIT | VK_BUFFER_USAGE_2_TRANSFER_SRC_BIT;
			svb_info.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
			svb_info.size = src->vertices.size() * sizeof(ModelSkinVertex);

			dst->skin_buffer = device->BufferAlloc(svb_info);
			dst->is_skinned = true;
		} else {
			dst->skin_buffer = gfx::ResourceKey::Null();
			dst->is_skinned = false;
		}

		dst->skin_index = src->skin_index;
	}

	asset->model.skeletons = load->skeletons;
	asset->model.clips = load->clips;
}

void ast::ModelLoader::UploadGPU(
	const LoaderContext &ctx,
	LoadResult *result,
	Asset *asset,
	const gfx::CmdBuffer &cmd,
	gfx::ResourceKey stage,
	u64 stage_offset
)
{
	ModelLoadData *load = static_cast<ModelLoadData *>(result->user_data);

	gfx::Device *device = gfx::Device::Get();

	u64 offset = stage_offset;

	for (int i = 0; i < asset->model.sub_models.size(); i++) {
		const ModelLoadSubModel *src = &load->submodels[i];
		SubModel *dst = &asset->model.sub_models[i];

		if (!src->skin_vertices.empty()) {
			u64 svb_size = src->skin_vertices.size() * sizeof(ModelSkinVertex);

			device->BufferWrite(stage, src->skin_vertices.data(), svb_size, offset);

			gfx::BufferCopy svb_copy;
			svb_copy.src_offset = offset;
			svb_copy.size = svb_size;

			cmd.CopyBufferToBuffer(stage, dst->skin_buffer, 1, &svb_copy);

			offset += svb_size;
		}
	}
}

void ast::ModelLoader::DestroyAsset(Asset *asset)
{
	gfx::Device *device = gfx::Device::Get();

	for (SubModel &sub_model : asset->model.sub_models) {
		if (!sub_model.skin_buffer.IsNull())
			device->BufferDestroy(sub_model.skin_buffer);
	}
}
