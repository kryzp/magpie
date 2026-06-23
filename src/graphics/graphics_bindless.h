#ifndef GRAPHICS_BINDLESS_H
#define GRAPHICS_BINDLESS_H

// TODO: Get this from the physical properties of the context.
#define G_BINDLESS_MAX_RESOURCES 4096
#define G_BINDLESS_INDEX_INVALID 0

typedef u32 G_BindlessIndex;

typedef struct G_BindlessHandle G_BindlessHandle;
struct G_BindlessHandle
{
	G_BindlessIndex value;
};

static inline G_BindlessIndex G_BindlessIndexOf(G_BindlessHandle handle)
{
	// Just return the value.
	return handle.value;
}

static inline b32 G_BindlessHandleValid(G_BindlessHandle handle)
{
	return (handle.value != 0) && (handle.value < G_BINDLESS_MAX_RESOURCES);
}

typedef enum G_BindlessKind
{
	G_BindlessKind_Sampler        = 0,
	G_BindlessKind_SampledTexture = 1,
	G_BindlessKind_SampledCubemap = 2,
	G_BindlessKind_StorageTexture = 3,
	G_BindlessKind_COUNT
}
G_BindlessKind;

typedef struct G_BindlessUpdate G_BindlessUpdate;
struct G_BindlessUpdate
{
	G_BindlessKind kind;
	u32 slot;
	VkSampler sampler;
	VkImageView view;
};

typedef struct G_Bindless G_Bindless;
struct G_Bindless
{
	VkDescriptorPool pool;
	VkDescriptorSetLayout layout;
	VkDescriptorSet set;

	u32 update_count;
	G_BindlessUpdate updates[512];

	G_BindlessIndex sampler_count;
	G_BindlessIndex view_count;

	G_BindlessIndex free_sampler_count;
	G_BindlessHandle free_samplers[128];

	G_BindlessIndex free_view_count;
	G_BindlessHandle free_views[128];
};

static VkDescriptorType G_BindlessGetVkType(G_BindlessKind kind);

static void G_BindlessPushUpdate(G_Bindless *bindless,
									 G_BindlessKind kind, G_BindlessHandle handle,
									 VkSampler sampler, VkImageView view);

static G_BindlessHandle G_BindlessRegisterSampler (G_Bindless *bindless, VkSampler sampler);
static G_BindlessHandle G_BindlessRegisterView    (G_Bindless *bindless, VkImageView view, b32 is_cubemap, b32 is_also_storage);

static void G_BindlessUpdateSampler (G_Bindless *bindless, G_BindlessHandle handle, VkSampler sampler);
static void G_BindlessUpdateView    (G_Bindless *bindless, G_BindlessHandle handle, VkImageView view, b32 is_cubemap, b32 is_also_storage);

static void G_BindlessFreeSampler (G_Bindless *bindless, G_BindlessHandle handle);
static void G_BindlessFreeView    (G_Bindless *bindless, G_BindlessHandle handle);

#endif // GRAPHICS_BINDLESS_H
