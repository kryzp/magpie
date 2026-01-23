#pragma once

#include "core/types.h"

#include "container/vector.h"

#include "entity.h"

namespace ent
{
	struct EntitySceneReference {
		u32 scene_id;
		u32 map_id;
	};

	struct MapEntity {
		EntityType type;
		u32 map_id;
		float x, y, z;
		void *settings;
	};

	class EntityScene {
	public:

		Vector<EntityScene> get_neighbouring_scenes();

	private:
		u32 scene_id;
		Vector<MapEntity> entities;
	};
}
