#ifndef GRAPHICS_BINDLESS_H
#define GRAPHICS_BINDLESS_H

#include "core/types.h"
#include "os/spin_lock.h"
#include "container/vector.h"

#include <volk/volk.h>

// TODO: Get this from the physical properties of the context.
#define GRAPHICS_BINDLESS_MAX_RESOURCES        4096
#define GRAPHICS_BINDLESS_MAX_FREE             1024
#define GRAPHICS_BINDLESS_INDEX_INVALID        0

namespace gfx {

inline bool BindlessIndexIsValid(unsigned index)
{
	return (index != 0) && (index < GRAPHICS_BINDLESS_MAX_RESOURCES);
}

enum BindlessKind {
	BindlessKind_Sampler = 0,
	BindlessKind_SampledTexture = 1,
	BindlessKind_SampledCubemap = 2,
	BindlessKind_StorageTexture = 3,
	BindlessKind_MaxEnum
};

VkDescriptorType BindlessKindToVk(BindlessKind kind);

struct BindlessUpdate {
	BindlessKind kind;
	u32 slot;
	VkSampler sampler;
	VkImageView view;
};

/*
 * slot reuse is only safe once there is no in-flight frame that can read
 * the old descriptor. therefore, frees come from the device's deferred destroy
 * queue, and shouldn't ever be from my user code.
 */
class Bindless {
public:
	Bindless();

	Bindless(const Bindless &) = delete;
	Bindless &operator=(const Bindless &) = delete;

	unsigned RegisterSampler(VkSampler sampler);
	unsigned RegisterView(VkImageView view, bool is_cubemap, bool is_also_storage);

	void UpdateSampler(unsigned index, VkSampler sampler);
	void UpdateView(unsigned index, VkImageView view, bool is_cubemap, bool is_also_storage);

	void FreeSampler(unsigned index);
	void FreeView(unsigned index);

	unsigned DrainUpdatesInto(BindlessUpdate *out, u64 out_capacity);

	VkDescriptorPool pool;
	VkDescriptorSetLayout layout;
	VkDescriptorSet set;

private:
	void PushUpdateLocked(BindlessKind kind, unsigned handle, VkSampler sampler, VkImageView view);
	void PushViewUpdatesLocked(unsigned handle, VkImageView view, bool is_cubemap, bool is_also_storage);

	static unsigned AllocSlotLocked(unsigned *free_slots, unsigned *free_count, unsigned *high_water);

	SpinLock lock;

	Vector<BindlessUpdate> updates;

	unsigned sampler_high_water; // 0 = null
	unsigned view_high_water;    // 0 = null

	unsigned free_sampler_count;
	unsigned free_samplers[GRAPHICS_BINDLESS_MAX_FREE];

	unsigned free_view_count;
	unsigned free_views[GRAPHICS_BINDLESS_MAX_FREE];
};
}

#endif // GRAPHICS_BINDLESS_H
