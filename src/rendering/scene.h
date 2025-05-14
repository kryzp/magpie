#pragma once

#include <vector>
#include <functional>
#include <array>

#include <glm/glm.hpp>

#include "math/transform.h"

#include "light.h"

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

		std::vector<RenderObject> &getRenderObjects();
		const std::vector<Mesh *> &getRenderList();

		void addLight(const Light &light);

		std::array<Light, MAX_POINT_LIGHTS> &getPointLights();
		const std::array<Light, MAX_POINT_LIGHTS> &getPointLights() const;

		int getPointLightCount() const;

	private:
		void aggregateMeshes();
		void sortRenderListByMaterialHash(int lo, int hi);

		std::vector<RenderObject> m_renderObjects;

		std::vector<Mesh *> m_renderList;
		bool m_renderListDirty;

		std::array<Light, MAX_POINT_LIGHTS> m_pointsLights;
		int m_pointLightCount;
	};
}
