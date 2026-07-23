#ifndef GRAPHICS_RESOURCE_H
#define GRAPHICS_RESOURCE_H

typedef union G_Resource G_Resource;
union G_Resource
{
	VkPipelineLayout pipeline_layout;
	VkPipeline pipeline;
	G_Texture texture;
	G_TextureView texture_view;
	G_Buffer buffer;
	G_Sampler sampler;
	G_ShaderProgram shader;
	G_AccelStruct accel_struct;
};
	
typedef struct G_ResourceListNode G_ResourceListNode;
struct G_ResourceListNode
{
	G_ResourceListNode *next;
	G_ResourceListNode *prev;
	
	G_ResourceKey key;
	G_Resource resource;
};

typedef struct G_ResourceList G_ResourceList;
struct G_ResourceList
{
	G_ResourceListNode first_sentinel;
	G_ResourceListNode free_sentinel;
};

internal void G_ResourceListInit(G_ResourceList *list);

internal G_ResourceKey G_ResourceListPush(G_ResourceList *list, Arena *arena, const G_Resource *resource, G_ResourceKey key);
internal G_ResourceKey G_ResourceListPushAuto(G_ResourceList *list, Arena *arena, const G_Resource *resource);

internal G_Resource *G_ResourceListGet(const G_ResourceList *list, G_ResourceKey key);

internal void G_ResourceListReturn(G_ResourceList *list, G_ResourceKey key);

#endif // GRAPHICS_RESOURCE_H
