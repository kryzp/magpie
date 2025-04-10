#include "scene.h"

#include "core/common.h"

#include "material.h"
#include "model.h"

using namespace mgp;

Scene::Scene()
	: m_renderObjects()
	, m_renderList()
	, m_renderListDirty(true)
	, m_pointsLights{}
	, m_pointLightCount(0)
{
}

Scene::~Scene()
{
}

void Scene::aggregateMeshes()
{
	m_renderList.clear();

	for (auto &obj : m_renderObjects)
	{
		if (!obj.model)
			continue;

		for (int i = 0; i < obj.model->getSubmeshCount(); i++)
		{
			m_renderList.push_back(obj.model->getSubmesh(i));
		}
	}
}

void Scene::sortRenderListByMaterialHash(int lo, int hi)
{
	if (lo >= hi || lo < 0)
		return;

	int pivot = m_renderList[hi]->getMaterial()->getHash();
	int i = lo - 1;

	for (int j = lo; j < hi; j++)
	{
		if (m_renderList[j]->getMaterial()->getHash() <= pivot)
		{
			i++;
			MGP_SWAP(m_renderList[i], m_renderList[j]);
		}
	}

	int partition = i + 1;
	MGP_SWAP(m_renderList[partition], m_renderList[hi]);

	sortRenderListByMaterialHash(lo, partition - 1);
	sortRenderListByMaterialHash(partition + 1, hi);
}

RenderObject *Scene::createRenderObject()
{
	m_renderListDirty = true;
	m_renderObjects.emplace_back();
	return &m_renderObjects.back(); // bad
}

void Scene::addPointLight(const PointLight& light)
{
	m_pointsLights[m_pointLightCount] = light;
	m_pointLightCount = (m_pointLightCount + 1) % MAX_POINT_LIGHTS;
}

void Scene::foreachObject(const std::function<bool(uint32_t, RenderObject &)> &fn)
{
	for (uint32_t i = 0; i < m_renderObjects.size(); i++)
	{
		if (!fn(i, m_renderObjects[i]))
			return;
	}
}

void Scene::foreachMesh(const std::function<bool(uint32_t, Mesh *)> &fn)
{
	auto list = getRenderList();

	for (uint32_t i = 0; i < list.size(); i++)
	{
		if (!fn(i, list[i]))
			return;
	}
}

const std::vector<RenderObject> &Scene::getRenderObjects() const
{
	return m_renderObjects;
}

const std::vector<Mesh *> &Scene::getRenderList()
{
	if (m_renderListDirty)
	{
		aggregateMeshes();
		sortRenderListByMaterialHash(0, m_renderList.size() - 1);

		m_renderListDirty = false;
	}

	return m_renderList;
}

const std::array<PointLight, MAX_POINT_LIGHTS> &Scene::getPointLights() const
{
	return m_pointsLights;
}
