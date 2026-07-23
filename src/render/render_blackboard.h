#ifndef RENDER_BLACKBOARD_H
#define RENDER_BLACKBOARD_H

typedef struct R_Blackboard R_Blackboard;
struct R_Blackboard
{
	R_GraphTexHandle lighting_resolve;
	R_GraphTexHandle lighting_msaa;

	R_GraphTexHandle depth_resolve;
	R_GraphTexHandle depth_msaa;
	
	//R_GraphTexHandle normals;

	u32 shadow_map_count;
	R_GraphTexHandle shadow_maps[R_FRAME_PARAMS_MAX_SHADOW_CASTERS];
	G_ResourceKey shadow_caster_table;
};

#endif // RENDER_BLACKBOARD_H
