#include "scene.h"

#include "core/common.h"

#include "material.h"
#include "mesh.h"

using namespace mgp;

RenderObject::RenderObject()
	: transform()
	, mesh()
{
}

RenderObject::~RenderObject()
{
}

Scene::Scene()
	: m_renderObjects()
	, m_renderList()
	, m_renderListDirty(true)
{
}

Scene::~Scene()
{
}

// todo: this shouldnt be called every frame. rather, createRenderObject should be addRenderObject(...) and when added we add all of its submeshes to the renderlist
void Scene::aggregateSubMeshes()
{
	m_renderList.clear();

	for (auto &obj : m_renderObjects)
	{
		if (!obj.mesh)
			continue;

		for (int i = 0; i < obj.mesh->getSubmeshCount(); i++)
		{
			m_renderList.push_back(obj.mesh->getSubmesh(i));
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

std::vector<RenderObject>::iterator Scene::createRenderObject()
{
	m_renderListDirty = true;
	m_renderObjects.emplace_back();
	return m_renderObjects.end();
}

const std::vector<SubMesh *> &Scene::getRenderList()
{
	if (m_renderListDirty)
	{
		aggregateSubMeshes();
		sortRenderListByMaterialHash(0, m_renderList.size() - 1);

		m_renderListDirty = false;
	}

	return m_renderList;
}
