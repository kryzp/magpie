#include "mesh.h"

using namespace llt;

Mesh::Mesh()
	: m_subMeshes()
{
}

Mesh::~Mesh()
{
	for (auto &sub : m_subMeshes) {
		delete sub;
	}

	m_subMeshes.clear();
}

SubMesh *Mesh::createSubmesh()
{
	SubMesh *sub = new SubMesh();
	sub->m_parent = this;
	m_subMeshes.pushBack(sub);
	return sub;
}

uint64_t Mesh::getSubmeshCount() const
{
	return m_subMeshes.size();
}

SubMesh *Mesh::getSubmesh(int idx) const
{
	return m_subMeshes[idx];
}

void Mesh::setOwner(RenderObject *owner)
{
	m_owner = owner;
}

RenderObject *Mesh::getOwner()
{
	return m_owner;
}
