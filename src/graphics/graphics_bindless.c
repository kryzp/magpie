
internal VkDescriptorType G_BindlessGetVkType(G_BindlessKind kind)
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

internal void G_BindlessPushUpdate(G_Bindless *bindless,
					   G_BindlessKind kind, u32 handle,
					   VkSampler sampler, VkImageView view)
{
	AssertTrue(G_BindlessIndexIsValid(handle));

	G_BindlessUpdate update = {0};
	update.kind = kind;
	update.slot = handle;
	update.sampler = sampler;
	update.view = view;

	bindless->updates[bindless->update_count] = update;
	bindless->update_count++;
}

internal u32 G_BindlessRegisterSampler(G_Bindless *bindless, VkSampler sampler)
{
	u32 index = 0;

	if (bindless->free_sampler_count > 0)
	{
		index = bindless->free_samplers[bindless->free_sampler_count];
		bindless->free_sampler_count--;
	}
	else
	{
		index = bindless->sampler_count + 1; // 0 reserved as null.
	}

	bindless->sampler_count++;

	G_BindlessUpdateSampler(bindless, index, sampler);

	return index;
}

internal u32 G_BindlessRegisterView(G_Bindless *bindless, VkImageView view, b32 is_cubemap, b32 is_also_storage)
{
	u32 index = 0;

	if (bindless->free_view_count > 0)
	{
		index = bindless->free_views[bindless->free_view_count];
		bindless->free_view_count--;
	}
	else
	{
		index = bindless->view_count + 1; // 0 reserved as null.
	}

	bindless->view_count++;

	G_BindlessUpdateView(bindless, index, view, is_cubemap, is_also_storage);

	return index;
}

internal void G_BindlessUpdateSampler(G_Bindless *bindless, u32 handle, VkSampler sampler)
{
	G_BindlessPushUpdate(bindless, G_BindlessKind_Sampler, handle, sampler, VK_NULL_HANDLE);
}

internal void G_BindlessUpdateView(G_Bindless *bindless, u32 handle, VkImageView view, b32 is_cubemap, b32 is_also_storage)
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

internal void G_BindlessFreeSampler(G_Bindless *bindless, u32 handle)
{
	AssertTrue(bindless->free_sampler_count < ArraySize(bindless->free_samplers));
	
	bindless->free_samplers[bindless->free_sampler_count] = handle;
	bindless->free_sampler_count++;
}

internal void G_BindlessFreeView(G_Bindless *bindless, u32 handle)
{
	AssertTrue(bindless->free_view_count < ArraySize(bindless->free_views));

	bindless->free_views[bindless->free_view_count] = handle;
	bindless->free_view_count++;
}
