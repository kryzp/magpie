#ifndef GRAPHICS_BINDLESS_H
#define GRAPHICS_BINDLESS_H

// TODO: Get this from the physical properties of the context.
#define GFX_BINDLESS_MAX_RESOURCES 4096

typedef struct GFX_BindlessHandle GFX_BindlessHandle;
struct GFX_BindlessHandle
{
	u32 value;
};

internal inline u32
GFX_BindlessIndexOf(GFX_BindlessHandle handle)
{
	// Just return the value.
	return handle.value;
}

internal inline b32
GFX_BindlessHandleValid(GFX_BindlessHandle handle)
{
	return (handle.value != 0) && (handle.value < GFX_BINDLESS_MAX_RESOURCES);
}

typedef enum GFX_BindlessSetKind
{
	GFX_BindlessSetKind_Sampler,
	GFX_BindlessSetKind_Sampled,
	GFX_BindlessSetKind_Storage,
	GFX_BindlessSetKind_COUNT
}
GFX_BindlessSetKind;

typedef struct GFX_BindlessUpdate GFX_BindlessUpdate;
struct GFX_BindlessUpdate
{
	GFX_BindlessSetKind kind;
	u32 slot;
	VkSampler sampler;
	VkImageView view;
};

typedef struct GFX_Bindless GFX_Bindless;
struct GFX_Bindless
{
	VkDescriptorPool pool;
	VkDescriptorSetLayout layouts[GFX_BindlessSetKind_COUNT];
	VkDescriptorSet sets[GFX_BindlessSetKind_COUNT];

	u32 update_count;
	GFX_BindlessUpdate updates[512];

	u32 sampler_count;
	u32 view_count;

	u32 free_sampler_count;
	GFX_BindlessHandle free_samplers[128];

	u32 free_view_count;
	GFX_BindlessHandle free_views[128];
};

internal VkDescriptorType GFX_BindlessGetVkType(GFX_BindlessSetKind kind);

internal void GFX_BindlessPushUpdate(GFX_Bindless *bindless,
									 GFX_BindlessSetKind kind, GFX_BindlessHandle handle,
									 VkSampler sampler, VkImageView view);

internal GFX_BindlessHandle GFX_BindlessRegisterSampler (GFX_Bindless *bindless, VkSampler sampler);
internal GFX_BindlessHandle GFX_BindlessRegisterView    (GFX_Bindless *bindless, VkImageView view, b32 is_also_storage);

internal void GFX_BindlessUpdateSampler (GFX_Bindless *bindless, GFX_BindlessHandle handle, VkSampler sampler);
internal void GFX_BindlessUpdateView    (GFX_Bindless *bindless, GFX_BindlessHandle handle, VkImageView view, b32 is_also_storage);

internal void GFX_BindlessFreeSampler (GFX_Bindless *bindless, GFX_BindlessHandle handle);
internal void GFX_BindlessFreeView    (GFX_Bindless *bindless, GFX_BindlessHandle handle);

#endif // GRAPHICS_BINDLESS_H
