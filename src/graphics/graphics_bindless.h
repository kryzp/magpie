#ifndef GRAPHICS_BINDLESS_H
#define GRAPHICS_BINDLESS_H

// TODO: Get this from the physical properties of the context.
#define G_BINDLESS_MAX_RESOURCES 4096
#define G_BINDLESS_MAX_UPDATES   1024
#define G_BINDLESS_MAX_FREE      1024
#define G_BINDLESS_INDEX_INVALID 0

internal inline b32 G_BindlessIndexIsValid(u32 index)
{
	return (index != 0) && (index < G_BINDLESS_MAX_RESOURCES);
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

/*
 * NOTE TO SELF:
 * slot reuse is only safe once there is no in-flight frame that can read
 * the old descriptor. therefore, frees come from the device's deferred destroy
 * queue, and shouldnt ever be from my user code.
 */

typedef struct G_Bindless G_Bindless;
struct G_Bindless
{
	VkDescriptorPool pool;
	VkDescriptorSetLayout layout;
	VkDescriptorSet set;

	i32 spinlock;

	u32 update_count;
	G_BindlessUpdate updates[G_BINDLESS_MAX_UPDATES];

	u32 sampler_high_water; // 0 = null
	u32 view_high_water; // 0 = null

	u32 free_sampler_count;
	u32 free_samplers[G_BINDLESS_MAX_FREE];

	u32 free_view_count;
	u32 free_views[G_BINDLESS_MAX_FREE];
};

internal VkDescriptorType G_BindlessGetVkType(G_BindlessKind kind);

internal u32 G_BindlessRegisterSampler(G_Bindless *bindless, VkSampler sampler);
internal u32 G_BindlessRegisterView(G_Bindless *bindless, VkImageView view, b32 is_cubemap, b32 is_also_storage);

internal void G_BindlessUpdateSampler(G_Bindless *bindless, u32 index, VkSampler sampler);
internal void G_BindlessUpdateView(G_Bindless *bindless, u32 index, VkImageView view, b32 is_cubemap, b32 is_also_storage);

internal void G_BindlessFreeSampler(G_Bindless *bindless, u32 index);
internal void G_BindlessFreeView(G_Bindless *bindless, u32 index);

internal u32 G_BindlessDrainUpdates(G_Bindless *bindless, G_BindlessUpdate *out);

#endif // GRAPHICS_BINDLESS_H
