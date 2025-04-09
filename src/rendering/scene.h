#pragma once

#include <vector>
#include <functional>

#include <glm/glm.hpp>

#include "math/transform.h"

namespace mgp
{
	class Model;
	class Mesh;

	struct RenderObject
	{
		Transform transform;
		Model *model;
	};

	class Scene
	{
	public:
		Scene();
		~Scene();

		RenderObject *createRenderObject();

		void foreachObject(const std::function<bool(uint32_t, RenderObject &)> &fn);
		void foreachMesh(const std::function<bool(uint32_t, Mesh *)> &fn);

		const std::vector<Mesh *> &getRenderList();

	private:
		void aggregateMeshes();
		void sortRenderListByMaterialHash(int lo, int hi);

		std::vector<RenderObject> m_renderObjects;

		std::vector<Mesh *> m_renderList;
		bool m_renderListDirty;
	};
}
