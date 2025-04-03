#include "scene.h"

#include "core/common.h"

#include "material.h"
#include "model.h"

using namespace mgp;

RenderObject::RenderObject()
	: transform()
	, model()
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

std::vector<RenderObject>::iterator Scene::createRenderObject()
{
	m_renderListDirty = true;
	m_renderObjects.emplace_back();
	return m_renderObjects.end();
}

void Scene::foreachObject(const std::function<void(RenderObject &)> &fn)
{
	for (auto &o : m_renderObjects)
	{
		fn(o);
	}
}

void Scene::foreachMesh(const std::function<void(Mesh &)> &fn)
{
	for (auto &o : m_renderObjects)
	{
		for (int i = 0; i < o.model->getSubmeshCount(); i++)
			fn(*o.model->getSubmesh(i));
	}
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
