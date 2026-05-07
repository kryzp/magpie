#ifndef RENDER_BLACKBOARD_H
#define RENDER_BLACKBOARD_H

/*
 * The render blackboard is just a collection of "global"
 * data used throughout a single frame of rendering.
 *
 * Different passes can write to this and then other
 * passes can read from it without having to have
 * three trillion different parameters into their functions.
 */

/*
typedef enum R_GBufferAttachment
{
	R_GBufferAttachment_Position,
	R_GBufferAttachment_Albedo,
	R_GBufferAttachment_Normal,
	R_GBufferAttachment_Emissive,
	R_GBufferAttachment_MetallicRoughness,
	R_GBufferAttachment_COUNT
}
R_GBufferAttachment;

typedef struct R_BB_GBufferData R_BB_GBufferData;
struct R_BB_GBufferData
{
	R_GraphTexHandle attachments[R_GBufferAttachment_COUNT];
	R_GraphTexHandle depth;
};
*/

typedef struct R_BB_ShadowData R_BB_ShadowData;
struct R_BB_ShadowData
{
	u32 shadow_map_count;
	R_GraphTexHandle shadow_maps[R_SCENE_MAX_SHADOW_CASTERS];
	
	GFX_BufferKey shadow_caster_table;
};

typedef struct R_Blackboard R_Blackboard;
struct R_Blackboard
{
	//R_BB_GBufferData gbuffer;
	R_GraphTexHandle depth;
	R_BB_ShadowData shadow_data;
};

#endif // RENDER_BLACKBOARD_H
