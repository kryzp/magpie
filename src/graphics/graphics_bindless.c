
internal VkDescriptorType
G_BindlessGetVkType(G_BindlessKind kind)
{
	switch (kind)
	{
		case G_BindlessKind_Sampler:         return VK_DESCRIPTOR_TYPE_SAMPLER;
		case G_BindlessKind_SampledTexture:  return VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
		case G_BindlessKind_SampledCubemap:  return VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
		case G_BindlessKind_StorageTexture:  return VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
	}

	AssertTrue(false && "Could not find descriptor type from bindless binding type.");

	return (VkDescriptorType)0;
}

internal void
G_BindlessPushUpdate(G_Bindless *bindless,
					   G_BindlessKind kind, G_BindlessHandle handle,
					   VkSampler sampler, VkImageView view)
{
	AssertTrue(G_BindlessHandleValid(handle));

	G_BindlessUpdate update = {0};
	update.kind = kind;
	update.slot = handle.value;
	update.sampler = sampler;
	update.view = view;

	bindless->updates[bindless->update_count] = update;
	bindless->update_count++;
}

internal G_BindlessHandle
G_BindlessRegisterSampler(G_Bindless *bindless, VkSampler sampler)
{
	G_BindlessHandle handle = {0};

	if (bindless->free_sampler_count > 0)
	{
		handle = bindless->free_samplers[bindless->free_sampler_count];
		bindless->free_sampler_count--;
	}
	else
	{
		handle.value = bindless->sampler_count + 1; // 0 reserved as null.
	}

	bindless->sampler_count++;

	G_BindlessUpdateSampler(bindless, handle, sampler);

	return handle;
}

internal G_BindlessHandle
G_BindlessRegisterView(G_Bindless *bindless, VkImageView view, b32 is_cubemap, b32 is_also_storage)
{
	G_BindlessHandle handle = {0};

	if (bindless->free_view_count > 0)
	{
		handle = bindless->free_views[bindless->free_view_count];
		bindless->free_view_count--;
	}
	else
	{
		handle.value = bindless->view_count + 1; // 0 reserved as null.
	}

	bindless->view_count++;

	G_BindlessUpdateView(bindless, handle, view, is_cubemap, is_also_storage);

	return handle;
}

internal void
G_BindlessUpdateSampler(G_Bindless *bindless, G_BindlessHandle handle, VkSampler sampler)
{
	G_BindlessPushUpdate(bindless, G_BindlessKind_Sampler, handle, sampler, VK_NULL_HANDLE);
}

internal void
G_BindlessUpdateView(G_Bindless *bindless, G_BindlessHandle handle, VkImageView view, b32 is_cubemap, b32 is_also_storage)
{
	G_BindlessKind kind = is_cubemap
		? G_BindlessKind_SampledCubemap
		: G_BindlessKind_SampledTexture;
	
	G_BindlessPushUpdate(bindless, kind, handle, VK_NULL_HANDLE, view);

	if (is_also_storage)
	{
		G_BindlessPushUpdate(bindless, G_BindlessKind_StorageTexture, handle, VK_NULL_HANDLE, view);
	}
}

internal void
G_BindlessFreeSampler(G_Bindless *bindless, G_BindlessHandle handle)
{
	AssertTrue(bindless->free_sampler_count < ArraySize(bindless->free_samplers));
	
	bindless->free_samplers[bindless->free_sampler_count] = handle;
	bindless->free_sampler_count++;
}

internal void
G_BindlessFreeView(G_Bindless *bindless, G_BindlessHandle handle)
{
	AssertTrue(bindless->free_view_count < ArraySize(bindless->free_views));

	bindless->free_views[bindless->free_view_count] = handle;
	bindless->free_view_count++;
}
