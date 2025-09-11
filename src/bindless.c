
// TODO: Implement a system where the bindless resources "defer"
//       updates to the next frame if they spill over the
//       BINDLESS_MAX_WRITES_PER_FRAME.

internal b32 BindlessIsValid(u32 resource_id)
{
	return resource_id != 0;
}

internal VkDescriptorType BindlessGetDescriptorTypeFromBinding(BindlessSetBinding binding)
{
	switch (binding) {
	case BindlessSetBinding_Sampler:  return VK_DESCRIPTOR_TYPE_SAMPLER;
	case BindlessSetBinding_Sampled:  return VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
	case BindlessSetBinding_Storage:  return VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
	}

	DebugLogCrash("Could not find descriptor type from binding type.");
	
	return (VkDescriptorType)0;
}

internal u32 BindlessPushUpdate(BindlessResources *bindless, BindlessUpdate *update)
{
	Assert(bindless->update_count < ArraySize(bindless->updates) &&
	       "Cannot add more bindless updates.");

	BindlessUpdate *p_update = bindless->updates + bindless->update_count;
	*p_update = *update;
	p_update->slot = ++bindless->resource_counts[p_update->type];
	
	bindless->update_count++;
	return p_update->slot;
}

internal u32 BindlessRegisterSampler(BindlessResources *bindless, VkSampler sampler)
{
	BindlessUpdate update = {0};
	update.type = BindlessSetBinding_Sampler;
	update.sampler.sampler = sampler;

	return BindlessPushUpdate(bindless, &update);
}

internal u32 BindlessRegisterSampled(BindlessResources *bindless, VkImageView view, b32 is_depth)
{
	BindlessUpdate update = {0};
	update.type = BindlessSetBinding_Sampled;
	update.sampled_image.view = view;
	update.sampled_image.is_depth = is_depth;

	return BindlessPushUpdate(bindless, &update);
}

internal u32 BindlessRegisterStorage(BindlessResources *bindless, VkImageView view)
{
	BindlessUpdate update = {0};
	update.type = BindlessSetBinding_Storage;
	update.storage_image.view = view;

	return BindlessPushUpdate(bindless, &update);
}

internal void BindlessInit(BindlessResources *bindless)
{
	VkDescriptorPoolSize pool_sizes[BindlessSetBinding_MaxEnum] = {0};
	
	for (u32 i = 0; i < BindlessSetBinding_MaxEnum; i++) {
		pool_sizes[i].type = BindlessGetDescriptorTypeFromBinding((BindlessSetBinding)i);
		pool_sizes[i].descriptorCount = BINDLESS_MAX_RESOURCES;
	}
	
	VkDescriptorPoolCreateInfo pool_create_info = {0};
	pool_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	pool_create_info.flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT_EXT;
	pool_create_info.maxSets = BINDLESS_MAX_RESOURCES * ArraySize(pool_sizes);
	pool_create_info.poolSizeCount = ArraySize(pool_sizes);
	pool_create_info.pPoolSizes = pool_sizes;
	
	VK_CHECK(vkCreateDescriptorPool(graphics_device->device,
					&pool_create_info, NULL,
					&bindless->pool),
		 "Failed to create bindless descriptor pool.");
	
	VkDescriptorBindingFlags bindless_flags = VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT_EXT | VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT_EXT;

	VkDescriptorSetLayoutBindingFlagsCreateInfo binding_flags = {0};
	binding_flags.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO;
	binding_flags.bindingCount = 1;
	binding_flags.pBindingFlags = &bindless_flags;
	
	for (u32 i = 0; i < BindlessSetBinding_MaxEnum; i++) {
		VkDescriptorSetLayoutBinding binding = {0};
		binding.descriptorType = BindlessGetDescriptorTypeFromBinding((BindlessSetBinding)i);
		binding.descriptorCount = BINDLESS_MAX_RESOURCES;
		binding.binding = 0;
		binding.stageFlags = VK_SHADER_STAGE_ALL;
		binding.pImmutableSamplers = NULL;
		
		VkDescriptorSetLayoutCreateInfo layout_create_info = {0};
		layout_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layout_create_info.bindingCount = 1;
		layout_create_info.pBindings = &binding;
		layout_create_info.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT_EXT;
		layout_create_info.pNext = &binding_flags;
		
		VK_CHECK(vkCreateDescriptorSetLayout(graphics_device->device,
						     &layout_create_info, NULL,
						     &bindless->layouts[i]),
			 "Failed to create bindless descriptor layout.");
	}
	
	// ---
	
	VkDescriptorSetAllocateInfo alloc_info = {0};
	alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	alloc_info.descriptorPool = bindless->pool;
	alloc_info.descriptorSetCount = BindlessSetBinding_MaxEnum;
	alloc_info.pSetLayouts = bindless->layouts;
	
	VK_CHECK(vkAllocateDescriptorSets(graphics_device->device, &alloc_info, bindless->sets),
		 "Failed to allocate bindless descriptor set.");
	
	DebugLog("Bindless resources created.");
}

internal void BindlessDestroy(BindlessResources *bindless)
{
	for (u32 i = 0; i < BindlessSetBinding_MaxEnum; i++)
		vkDestroyDescriptorSetLayout(graphics_device->device, bindless->layouts[i], NULL);
	
	vkDestroyDescriptorPool(graphics_device->device, bindless->pool, NULL);
}

internal void BindlessApplyUpdates(BindlessResources *bindless)
{
	VkWriteDescriptorSet descriptor_writes[BINDLESS_MAX_WRITES_PER_FRAME] = {0};
	VkDescriptorImageInfo image_infos[BINDLESS_MAX_WRITES_PER_FRAME] = {0};

	for (u32 i = 0; i < bindless->update_count; i++) {
		BindlessUpdate *update = bindless->updates + i;

		VkDescriptorImageInfo *image_info = image_infos + i;
		
		VkWriteDescriptorSet *write = descriptor_writes + i;
		write->sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		write->descriptorCount = 1;
		write->dstArrayElement = update->slot;
		write->descriptorType = BindlessGetDescriptorTypeFromBinding(update->type);
		write->dstSet = bindless->sets[update->type];
		write->dstBinding = 0;
		write->pImageInfo = image_info;

		switch (update->type) {
		case BindlessSetBinding_Sampler:
			image_info->sampler = update->sampler.sampler;
			image_info->imageView = VK_NULL_HANDLE;
			image_info->imageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			break;

		case BindlessSetBinding_Sampled:
			image_info->sampler = VK_NULL_HANDLE;
			image_info->imageView = update->sampled_image.view;
			image_info->imageLayout = update->sampled_image.is_depth
				? VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL
				: VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			break;

		case BindlessSetBinding_Storage:
			image_info->sampler = VK_NULL_HANDLE;
			image_info->imageView = update->storage_image.view;
			image_info->imageLayout = VK_IMAGE_LAYOUT_GENERAL;
			break;
		}
	}

	if (bindless->update_count > 0) {
		vkUpdateDescriptorSets(graphics_device->device,
				       bindless->update_count, descriptor_writes,
				       0, NULL);
		bindless->update_count = 0;
	}
}
