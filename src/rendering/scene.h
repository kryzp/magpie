#pragma once

#include <vector>
#include <functional>

#include <glm/glm.hpp>

#include "math/transform.h"

namespace mgp
{
	class Model;
	class Mesh;

	class RenderObject
	{
	public:
		RenderObject();
		~RenderObject();

		Transform transform;
		Model *model;
	};

	class Scene
	{
	public:
		Scene();
		~Scene();

		std::vector<RenderObject>::iterator createRenderObject();

		void foreachObject(const std::function<void(RenderObject &)> &fn);
		void foreachMesh(const std::function<void(Mesh &)> &fn);

		const std::vector<Mesh *> &getRenderList();

	private:
		void aggregateMeshes();
		void sortRenderListByMaterialHash(int lo, int hi);

		std::vector<RenderObject> m_renderObjects;

		std::vector<Mesh *> m_renderList;
		bool m_renderListDirty;
	};
}
