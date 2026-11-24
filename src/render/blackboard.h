#ifndef RENDER_BLACKBOARD_H
#define RENDER_BLACKBOARD_H

#include "core/types.h"
#include "render/frame.h"
#include "render/graph/graph_types.h"

namespace rnd {

struct Blackboard {
	GraphTexHandle lighting_resolve;
	GraphTexHandle lighting_msaa;

	GraphTexHandle depth_resolve;
	GraphTexHandle depth_msaa;

	//GraphTexHandle normals;

	unsigned shadow_map_count = 0;
	GraphTexHandle shadow_maps[RENDER_FRAME_PARAMS_MAX_SHADOW_CASTERS];
	u64 shadow_caster_table_addr = 0;
};

}

#endif // RENDER_BLACKBOARD_H
