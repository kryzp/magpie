#ifndef SCENE_H_
#define SCENE_H_

#include "container/vector.h"

#include "render_object.h"

namespace llt
{
	class Camera;
	class RenderTarget;
	class CommandBuffer;
	class SubMesh;

	class Scene
	{
	public:
		Scene();
		~Scene();

		void updatePrevMatrices();

		Vector<RenderObject>::Iterator createRenderObject();

		const Vector<SubMesh *> &getRenderList();

	private:
		void aggregateSubMeshes();
		void sortRenderListByMaterialHash(int lo, int hi);

		Vector<RenderObject> m_renderObjects;

		Vector<SubMesh *> m_renderList;
		bool m_renderListDirty;
	};
}

#endif // SCENE_H_
