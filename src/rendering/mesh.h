#ifndef MODEL_H_
#define MODEL_H_

#include "container/string.h"
#include "container/vector.h"

#include "sub_mesh.h"

namespace llt
{
	class RenderObject;

	class Mesh
	{
	public:
		Mesh();
		~Mesh();

		SubMesh *createSubmesh();

		uint64_t getSubmeshCount() const;
		SubMesh *getSubmesh(int idx) const;

		void setOwner(RenderObject *owner);
		RenderObject *getOwner();

		void setDirectory(const String &directory);
		const String &getDirectory() const;

	private:
		RenderObject *m_owner;
		Vector<SubMesh*> m_subMeshes;
		String m_directory;
	};
}

#endif // MODEL_H_
