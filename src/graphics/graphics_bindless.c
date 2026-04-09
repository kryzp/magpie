
internal VkDescriptorType
GFX_BindlessGetVkType(GFX_BindlessSetKind kind)
{
	switch (kind)
	{
		case GFX_BindlessSetKind_Sampler:  return VK_DESCRIPTOR_TYPE_SAMPLER;
		case GFX_BindlessSetKind_Sampled:  return VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
		case GFX_BindlessSetKind_Storage:  return VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
	}

	AssertTrue(false && "Could not find descriptor type from bindless binding type.");

	return (VkDescriptorType)0;
}

internal void
GFX_BindlessPushUpdate(GFX_Bindless *bindless,
					   GFX_BindlessSetKind kind, GFX_BindlessHandle handle,
					   VkSampler sampler, VkImageView view)
{
	AssertTrue(GFX_BindlessHandleValid(handle));

	GFX_BindlessUpdate update = {0};
	update.kind = kind;
	update.slot = handle.value;
	update.sampler = sampler;
	update.view = view;

	bindless->updates[bindless->update_count] = update;
	bindless->update_count++;
}

internal GFX_BindlessHandle
GFX_BindlessRegisterSampler(GFX_Bindless *bindless, VkSampler sampler)
{
	GFX_BindlessHandle handle = {0};

	if (bindless->free_sampler_count > 0)
	{
		handle = bindless->free_samplers[bindless->free_sampler_count];
		bindless->free_sampler_count--;
	}
	else
	{
		handle.value = bindless->sampler_count;
	}

	bindless->sampler_count++;

	GFX_BindlessUpdateSampler(bindless, handle, sampler);

	return handle;
}

internal GFX_BindlessHandle
GFX_BindlessRegisterView(GFX_Bindless *bindless, VkImageView view, b32 is_also_storage)
{
	GFX_BindlessHandle handle = {0};

	if (bindless->free_view_count > 0)
	{
		handle = bindless->free_views[bindless->free_view_count];
		bindless->free_view_count--;
	}
	else
	{
		handle.value = bindless->view_count;
	}

	bindless->view_count++;

	GFX_BindlessUpdateView(bindless, handle, view, is_also_storage);

	return handle;
}

internal void
GFX_BindlessUpdateSampler(GFX_Bindless *bindless, GFX_BindlessHandle handle, VkSampler sampler)
{
	GFX_BindlessPushUpdate(bindless, GFX_BindlessSetKind_Sampler, handle, sampler, VK_NULL_HANDLE);
}

internal void
GFX_BindlessUpdateView(GFX_Bindless *bindless, GFX_BindlessHandle handle, VkImageView view, b32 is_also_storage)
{
	GFX_BindlessPushUpdate(bindless, GFX_BindlessSetKind_Sampled, handle, VK_NULL_HANDLE, view);

	if (is_also_storage)
		GFX_BindlessPushUpdate(bindless, GFX_BindlessSetKind_Storage, handle, VK_NULL_HANDLE, view);
}

internal void
GFX_BindlessFreeSampler(GFX_Bindless *bindless, GFX_BindlessHandle handle)
{
	AssertTrue(bindless->free_sampler_count < ArraySize(bindless->free_samplers));
	
	bindless->free_samplers[bindless->free_sampler_count] = handle;
	bindless->free_sampler_count++;
}

internal void
GFX_BindlessFreeView(GFX_Bindless *bindless, GFX_BindlessHandle handle)
{
	AssertTrue(bindless->free_view_count < ArraySize(bindless->free_views));

	bindless->free_views[bindless->free_view_count] = handle;
	bindless->free_view_count++;
}
