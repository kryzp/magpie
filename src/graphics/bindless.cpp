#include "graphics/bindless.h"
#include "core/log.h"
#include "core/maths.h"

VkDescriptorType gfx::BindlessKindToVk(BindlessKind kind)
{
	switch (kind) {
		case BindlessKind_Sampler:
			return VK_DESCRIPTOR_TYPE_SAMPLER;
		case BindlessKind_SampledTexture:
			return VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
		case BindlessKind_SampledCubemap:
			return VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
		case BindlessKind_StorageTexture:
			return VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		default:
			break;
	}

	AssertTrue(false && "Could not find descriptor type from bindless binding type.");

	return (VkDescriptorType)0;
}

gfx::Bindless::Bindless()
	: pool(VK_NULL_HANDLE)
	, layout(VK_NULL_HANDLE)
	, set(VK_NULL_HANDLE)
	, updates()
	, sampler_high_water(0)
	, view_high_water(0)
	, free_sampler_count(0)
	, free_view_count(0)
{
}

// CALLER HOLDS LOCK
void gfx::Bindless::PushUpdateLocked(BindlessKind kind, unsigned handle, VkSampler sampler, VkImageView view)
{
	AssertTrue(BindlessIndexIsValid(handle));

	BindlessUpdate update = {};
	update.kind = kind;
	update.slot = handle;
	update.sampler = sampler;
	update.view = view;

	updates.push_back(update);
}

// CALLER HOLDS LOCK
void gfx::Bindless::PushViewUpdatesLocked(unsigned handle, VkImageView view, bool is_cubemap, bool is_also_storage)
{
	BindlessKind kind = is_cubemap
		? BindlessKind_SampledCubemap
		: BindlessKind_SampledTexture;

	PushUpdateLocked(kind, handle, VK_NULL_HANDLE, view);

	if (is_also_storage)
		PushUpdateLocked(BindlessKind_StorageTexture, handle, VK_NULL_HANDLE, view);
}

// CALLER HOLDS LOCK
unsigned gfx::Bindless::AllocSlotLocked(unsigned *free_slots, unsigned *free_count, unsigned *high_water)
{
	if (*free_count > 0) {
		(*free_count)--;
		return free_slots[*free_count];
	}

	(*high_water)++;

	unsigned index = *high_water;

	DebugPrintAssert(BindlessIndexIsValid(index), "Out of bindless slots.");

	return index;
}

unsigned gfx::Bindless::RegisterSampler(VkSampler sampler)
{
	SpinLockScope scope(lock);
	unsigned index = AllocSlotLocked(free_samplers, &free_sampler_count, &sampler_high_water);
	PushUpdateLocked(BindlessKind_Sampler, index, sampler, VK_NULL_HANDLE);
	return index;
}

unsigned gfx::Bindless::RegisterView(VkImageView view, bool is_cubemap, bool is_also_storage)
{
	SpinLockScope scope(lock);
	unsigned index = AllocSlotLocked(free_views, &free_view_count, &view_high_water);
	PushViewUpdatesLocked(index, view, is_cubemap, is_also_storage);
	return index;
}

void gfx::Bindless::UpdateSampler(unsigned handle, VkSampler sampler)
{
	SpinLockScope scope(lock);
	PushUpdateLocked(BindlessKind_Sampler, handle, sampler, VK_NULL_HANDLE);
}

void gfx::Bindless::UpdateView(unsigned handle, VkImageView view, bool is_cubemap, bool is_also_storage)
{
	SpinLockScope scope(lock);
	PushViewUpdatesLocked(handle, view, is_cubemap, is_also_storage);
}

void gfx::Bindless::FreeSampler(unsigned handle)
{
	if (!BindlessIndexIsValid(handle))
		return;

	SpinLockScope scope(lock);
	DebugPrintAssert(free_sampler_count < ArraySize(free_samplers), "Bindless sampler free list is full.");
	free_samplers[free_sampler_count++] = handle;
}

void gfx::Bindless::FreeView(unsigned handle)
{
	if (!BindlessIndexIsValid(handle))
		return;

	SpinLockScope scope(lock);
	DebugPrintAssert(free_view_count < ArraySize(free_views), "Bindless view free list is full.");
	free_views[free_view_count++] = handle;
}

unsigned gfx::Bindless::DrainUpdatesInto(BindlessUpdate *out, u64 out_capacity)
{
	SpinLockScope scope(lock);

	unsigned count = maths::Min(updates.size(), out_capacity);
	memcpy(out, updates.data(), count * sizeof(BindlessUpdate));
	updates.erase(updates.begin(), updates.begin() + count);

	return count;
}
