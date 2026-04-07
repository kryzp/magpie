#ifndef RENDER_BLACKBOARD_H
#define RENDER_BLACKBOARD_H

typedef struct R_BB_GBufferData R_BB_GBufferData;
struct R_BB_GBufferData
{
	R_GraphTexHandle attachments[5];
	R_GraphTexHandle depth;
};

typedef struct R_BB_ShadowData R_BB_ShadowData;
struct R_BB_ShadowData
{
	u32 shadow_map_count;
	R_GraphTexHandle shadow_maps[8];
	GFX_BufferKey shadow_caster_table;
};

typedef struct R_Blackboard R_Blackboard;
struct R_Blackboard
{
	R_BB_GBufferData gbuffer;
	R_BB_ShadowData shadow_data;
};

#endif // RENDER_BLACKBOARD_H
