#include "render/model.h"
#include "asset/asset_system.h"

rnd::Model rnd::Model::FromAsset(Scene *scene, ast::Handle asset_handle)
{
	const ast::ModelAsset *model_asset = &ast::AssetSystem::Get()->GetOrBreak(asset_handle)->model;

	Model model;
	model.asset_handle = asset_handle;
	model.submodels.reserve(model_asset->sub_models.size());

	for (const ast::SubModel &asset_src : model_asset->sub_models) {
		SubModel submodel;

		MeshDesc mesh_desc;

		mesh_desc.vertices = asset_src.vertices.data();
		mesh_desc.vertex_count = asset_src.vertices.size();
		mesh_desc.vertex_stride = asset_src.vertex_stride;
		mesh_desc.indices = asset_src.indices.data();
		mesh_desc.index_count = asset_src.indices.size();
		mesh_desc.index_type = asset_src.index_type;
		mesh_desc.skin_buffer = asset_src.skin_buffer;

		submodel.mesh = scene->AllocMesh(mesh_desc);

		submodel.material = scene->AddMaterialFromAssets(asset_src.material);
		submodel.local_transform = asset_src.transform;
		submodel.skin_index = asset_src.skin_index;

		Vec3 bounds_min = asset_src.bounds_min;
		Vec3 bounds_max = asset_src.bounds_max;
		Vec3 local_centre = (bounds_min + bounds_max) * 0.5f;
		Vec3 dx = bounds_max - bounds_min;
		float local_radius = dx.Length() * 0.5f;

		submodel.local_sphere_bounds = Vec4(
			local_centre.x,
			local_centre.y,
			local_centre.z,
			local_radius
		);

		model.submodels.push_back(submodel);
	}

	return model;
}

rnd::ModelCatalogue::ModelCatalogue()
	: equipped_scene(nullptr)
{
}

rnd::ModelCatalogue::~ModelCatalogue()
{
	for (const ModelCatalogueEntry &entry : entries)
		FreeModelResources(entry.model);
}

rnd::ModelCatalogueEntry *rnd::ModelCatalogue::TryFindEntry(ast::Handle asset_handle)
{
	for (ModelCatalogueEntry &entry : entries) {
		if (entry.asset_handle == asset_handle)
			return &entry;
	}

	return nullptr;
}

void rnd::ModelCatalogue::FreeModelResources(const Model &model)
{
	for (const SubModel &submodel : model.submodels) {
		equipped_scene->FreeMesh(submodel.mesh);
		equipped_scene->FreeMaterial(submodel.material);
	}
}

void rnd::ModelCatalogue::EquipScene(Scene *scene)
{
	equipped_scene = scene;
}

rnd::Model *rnd::ModelCatalogue::CreateModel(ast::Handle asset_handle)
{
	ModelCatalogueEntry *existing = TryFindEntry(asset_handle);

	if (existing) {
		existing->ref_count++;
		return &existing->model;
	}

	entries.emplace_front();

	entries.front().asset_handle = asset_handle;
	entries.front().model = Model::FromAsset(equipped_scene, asset_handle);
	entries.front().ref_count = 1;

	return &entries.front().model;
}

void rnd::ModelCatalogue::ReleaseModel(ast::Handle asset_handle)
{
	for (auto it = entries.begin(); it != entries.end(); ++it) {
		if (it->asset_handle != asset_handle)
			continue;

		if (it->ref_count > 0)
			it->ref_count--;

		if (it->ref_count == 0) {
			FreeModelResources(it->model);
			entries.erase(it);
		}

		return;
	}
}

rnd::Model *rnd::ModelCatalogue::TryFindModel(ast::Handle asset_handle)
{
	ModelCatalogueEntry *entry = TryFindEntry(asset_handle);

	if (!entry)
		return nullptr;

	return &entry->model;
}

rnd::ModelInstance rnd::ModelCatalogue::CreateInstance(ast::Handle asset_handle, const Mat4 &initial_transform)
{
	Model *model = CreateModel(asset_handle);

	anim::AnimationSystem *animation = anim::AnimationSystem::Get();

	ModelInstance instance;
	instance.asset_handle = asset_handle;
	instance.root_transform = initial_transform;
	instance.animator_handle = anim::Handle();

	bool needs_animator = false;

	for (const SubModel &submodel : model->submodels) {
		if (submodel.skin_index >= 0) {
			needs_animator = true;
			break;
		}
	}

	if (needs_animator) {
		instance.has_animator = true;
		instance.animator_handle = animation->CreateInstance(asset_handle);
	}

	instance.submodels.reserve(model->submodels.size());

	for (const SubModel &submodel : model->submodels) {
		EntityHandle handle = equipped_scene->EntityCreate(EntityType_Object);

		equipped_scene->SetObjectMesh(handle, submodel.mesh);
		equipped_scene->SetObjectMaterial(handle, submodel.material);
		equipped_scene->SetObjectLocalSphereBounds(handle, submodel.local_sphere_bounds);

		if (instance.has_animator) {
			anim::Palette palette = animation->GetPalette(instance.animator_handle, submodel.skin_index);
			equipped_scene->SetObjectSkinning(handle, palette.matrices, palette.joint_count);
		}

		instance.submodels.push_back(handle);
	}

	SetInstanceTransform(&instance, initial_transform);

	return instance;
}

rnd::ModelInstance rnd::ModelCatalogue::CreateInstanceFromPath(const String &asset_path, const Mat4 &initial_transform)
{
	ast::Handle asset_handle = ast::AssetSystem::Get()->RequireAssetBlocking(asset_path);
	AssertTrue(asset_handle.type == ast::AssetType_Model && "Attempting to load model asset and it's not a model!!!");
	return CreateInstance(asset_handle, initial_transform);
}

void rnd::ModelCatalogue::DestroyInstance(ModelInstance *instance)
{
	for (EntityHandle handle : instance->submodels)
		equipped_scene->EntityDestroy(handle);

	instance->submodels.clear();

	if (instance->has_animator)
		anim::AnimationSystem::Get()->KillInstance(instance->animator_handle);

	ReleaseModel(instance->asset_handle);
}

void rnd::ModelCatalogue::SetInstanceTransform(ModelInstance *instance, const Mat4 &root_transform)
{
	Model *original = TryFindModel(instance->asset_handle);

	instance->root_transform = root_transform;

	int count = maths::Min(original->submodels.size(), instance->submodels.size());

	for (int i = 0; i < count; i++) {
		Mat4 world_matrix = instance->root_transform * original->submodels[i].local_transform;
		equipped_scene->SetObjectTransform(instance->submodels[i], world_matrix);
	}
}
