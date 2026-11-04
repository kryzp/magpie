
internal VkDescriptorType G_BindlessGetVkType(G_BindlessKind kind)
{
	switch (kind)
	{
		case G_BindlessKind_Sampler:         return VK_DESCRIPTOR_TYPE_SAMPLER;
		case G_BindlessKind_SampledTexture:  return VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
		case G_BindlessKind_SampledCubemap:  return VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
		case G_BindlessKind_StorageTexture:  return VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		default: break;
	}

	AssertTrue(false && "Could not find descriptor type from bindless binding type.");

	return (VkDescriptorType)0;
}

// Caller holds the lock.
internal void G_BindlessPushUpdateLocked(G_Bindless *bindless,
										 G_BindlessKind kind, u32 handle,
										 VkSampler sampler, VkImageView view)
{
	AssertTrue(G_BindlessIndexIsValid(handle));
	AssertTrue(bindless->update_count < ArraySize(bindless->updates));

	G_BindlessUpdate update = {0};
	update.kind = kind;
	update.slot = handle;
	update.sampler = sampler;
	update.view = view;

	bindless->updates[bindless->update_count++] = update;
}

// Caller holds the lock.
internal void G_BindlessPushViewUpdatesLocked(G_Bindless *bindless, u32 handle, VkImageView view, b32 is_cubemap, b32 is_also_storage)
{
	G_BindlessKind kind = is_cubemap
		? G_BindlessKind_SampledCubemap
		: G_BindlessKind_SampledTexture;

	G_BindlessPushUpdateLocked(bindless, kind, handle, VK_NULL_HANDLE, view);

	if (is_also_storage)
		G_BindlessPushUpdateLocked(bindless, G_BindlessKind_StorageTexture, handle, VK_NULL_HANDLE, view);
}

// Caller holds the lock.
internal u32 G_BindlessAllocSlotLocked(u32 *free_slots, u32 *free_count, u32 *high_water)
{
	if (*free_count > 0) {
		(*free_count)--;
		return free_slots[*free_count];
	}

	(*high_water)++;

	u32 index = *high_water;

	// todo: use debug log assert!!!!
	AssertTrue(G_BindlessIndexIsValid(index) && "Out of bindless slots.");

	return index;
}

internal u32 G_BindlessRegisterSampler(G_Bindless *bindless, VkSampler sampler)
{
	osapi->SpinLockAcquire(&bindless->spinlock);

	u32 index = G_BindlessAllocSlotLocked(bindless->free_samplers,
										  &bindless->free_sampler_count,
										  &bindless->sampler_high_water);

	G_BindlessPushUpdateLocked(bindless, G_BindlessKind_Sampler, index, sampler, VK_NULL_HANDLE);

	osapi->SpinLockRelease(&bindless->spinlock);

	return index;
}

internal u32 G_BindlessRegisterView(G_Bindless *bindless, VkImageView view, b32 is_cubemap, b32 is_also_storage)
{
	osapi->SpinLockAcquire(&bindless->spinlock);

	u32 index = G_BindlessAllocSlotLocked(bindless->free_views,
										  &bindless->free_view_count,
										  &bindless->view_high_water);

	G_BindlessPushViewUpdatesLocked(bindless, index, view, is_cubemap, is_also_storage);

	osapi->SpinLockRelease(&bindless->spinlock);

	return index;
}

internal void G_BindlessUpdateSampler(G_Bindless *bindless, u32 handle, VkSampler sampler)
{
	osapi->SpinLockAcquire(&bindless->spinlock);
	G_BindlessPushUpdateLocked(bindless, G_BindlessKind_Sampler, handle, sampler, VK_NULL_HANDLE);
	osapi->SpinLockRelease(&bindless->spinlock);
}

internal void G_BindlessUpdateView(G_Bindless *bindless, u32 handle, VkImageView view, b32 is_cubemap, b32 is_also_storage)
{
	osapi->SpinLockAcquire(&bindless->spinlock);
	G_BindlessPushViewUpdatesLocked(bindless, handle, view, is_cubemap, is_also_storage);
	osapi->SpinLockRelease(&bindless->spinlock);
}

internal void G_BindlessFreeSampler(G_Bindless *bindless, u32 handle)
{
	if (!G_BindlessIndexIsValid(handle))
		return;

	osapi->SpinLockAcquire(&bindless->spinlock);
	AssertTrue(bindless->free_sampler_count < ArraySize(bindless->free_samplers)); // todo: use debug log assert!!!!
	bindless->free_samplers[bindless->free_sampler_count++] = handle;
	osapi->SpinLockRelease(&bindless->spinlock);
}

internal void G_BindlessFreeView(G_Bindless *bindless, u32 handle)
{
	if (!G_BindlessIndexIsValid(handle))
		return;

	osapi->SpinLockAcquire(&bindless->spinlock);
	AssertTrue(bindless->free_view_count < ArraySize(bindless->free_views)); // todo: use debug log assert!!!!
	bindless->free_views[bindless->free_view_count++] = handle;
	osapi->SpinLockRelease(&bindless->spinlock);
}

internal u32 G_BindlessDrainUpdates(G_Bindless *bindless, G_BindlessUpdate *out)
{
	// todo: somehow assert that out is maximum G_BINDLESS_MAX_UPDATES in size.
	
	osapi->SpinLockAcquire(&bindless->spinlock);

	u32 count = MinValue(bindless->update_count, G_BINDLESS_MAX_UPDATES);
	MemCopy(out, bindless->updates, count * sizeof(G_BindlessUpdate));
	bindless->update_count -= count;

	osapi->SpinLockRelease(&bindless->spinlock);

	return count;
}
