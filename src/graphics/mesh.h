#ifndef MODEL_H_
#define MODEL_H_

#include "../container/vector.h"

#include "sub_mesh.h"

namespace llt
{
	class Mesh
	{
	public:
		Mesh();
		~Mesh();

		SubMesh* createSubmesh();

		uint64_t getSubmeshCount() const;
		SubMesh* getSubmesh(int idx) const;

	private:
		Vector<SubMesh*> m_subMeshes;
	};
}

#endif // MODEL_H_
