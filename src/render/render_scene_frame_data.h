#ifndef RENDER_SCENE_FRAME_DATA_H
#define RENDER_SCENE_FRAME_DATA_H

typedef struct R_SceneFrameData R_SceneFrameData;
struct R_SceneFrameData
{
	u32 page_count;
	u32 object_count;
	
	G_Alloc object_buffer;
	G_Alloc light_buffer;
	G_Alloc page_table_buffer;
	G_Alloc skinning_palette_buffer;

	u32 shadow_caster_count;
	R_ShadowCaster *shadow_casters;
};

static R_SceneFrameData R_SceneUploadFrameData(R_Scene *scene, G_RingBuffer *ring);
static void             R_SceneUploadPageTable(R_Scene *scene, G_RingBuffer *ring, R_SceneFrameData *out);
static void             R_SceneUploadSkinning(R_Scene *scene, G_RingBuffer *ring, R_SceneFrameData *out);
static void             R_SceneUploadObjects(R_Scene *scene, G_RingBuffer *ring, R_SceneFrameData *out);
static void             R_SceneUploadLights(R_Scene *scene, G_RingBuffer *ring, R_SceneFrameData *out);

#endif // RENDER_SCENE_FRAME_DATA_H
