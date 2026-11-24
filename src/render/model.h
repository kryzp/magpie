#ifndef RENDER_MODEL_H
#define RENDER_MODEL_H

#include "core/types.h"
#include "core/maths.h"
#include "asset/handle.h"
#include "animation/animation_system.h"
#include "render/scene.h"

#include <list>
#include "container/string.h"
#include "container/vector.h"

namespace rnd {

struct SubModel {
	MeshHandle mesh;
	MaterialHandle material;

	Mat4 local_transform; // relative to root
	Vec4 local_sphere_bounds;

	int skin_index = -1;
};

struct Model {
	ast::Handle asset_handle;
	Vector<SubModel> submodels;

	static Model FromAsset(Scene *scene, ast::Handle asset_handle);
};

struct ModelInstance {
	ast::Handle asset_handle;
	Vector<EntityHandle> submodels;
	Mat4 root_transform;
	bool has_animator = false;
	anim::Handle animator_handle;
};

struct ModelCatalogueEntry {
	ast::Handle asset_handle;
	Model model;
	unsigned ref_count = 0;
};

class ModelCatalogue {
public:
	ModelCatalogue();
	~ModelCatalogue();

	ModelCatalogue(const ModelCatalogue &) = delete;
	ModelCatalogue &operator=(const ModelCatalogue &) = delete;

	void EquipScene(Scene *scene);

	Scene *GetEquippedScene() const
	{
		return equipped_scene;
	}

	Model *CreateModel(ast::Handle asset_handle);
	void ReleaseModel(ast::Handle asset_handle);

	Model *TryFindModel(ast::Handle asset_handle);

	/* ==================================================
	   INSTANCES
	   ================================================== */

	ModelInstance CreateInstance(ast::Handle asset_handle, const Mat4 &initial_transform);
	ModelInstance CreateInstanceFromPath(const String &asset_path, const Mat4 &initial_transform);

	void DestroyInstance(ModelInstance *instance);

	void SetInstanceTransform(ModelInstance *instance, const Mat4 &root_transform);

private:
	ModelCatalogueEntry *TryFindEntry(ast::Handle asset_handle);
	void FreeModelResources(const Model &model);

	Scene *equipped_scene;

	std::list<ModelCatalogueEntry> entries;
};

}

#endif // RENDER_MODEL_H
