#pragma once

#include <vector>

#include <glm/glm.hpp>

#include "math/transform.h"

namespace mgp
{
	class Mesh;
	class SubMesh;

	class RenderObject
	{
	public:
		RenderObject();
		~RenderObject();

		Transform transform;
		Mesh *mesh;
	};

	class Scene
	{
	public:
		Scene();
		~Scene();

		std::vector<RenderObject>::iterator createRenderObject();

		const std::vector<SubMesh *> &getRenderList();

	private:
		void aggregateSubMeshes();
		void sortRenderListByMaterialHash(int lo, int hi);

		std::vector<RenderObject> m_renderObjects;

		std::vector<SubMesh *> m_renderList;
		bool m_renderListDirty;
	};
}
