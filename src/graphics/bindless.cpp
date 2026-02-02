#include "bindless.h"

using namespace gfx;

BindlessResources::BindlessResources()
	: pool()
	, layouts{}
	, sets{}
	, resource_counts{}
	, updates()
{
}

BindlessResources::~BindlessResources()
{
}

BindlessHandle BindlessResources::push_update(BindlessSetKind kind, VkSampler sampler, VkImageView view)
{
	BindlessUpdate update = {};
	update.kind = kind;
	update.sampler = sampler;
	update.view = view;
	update.slot = ++resource_counts[kind];
	
	assert(update.slot < BindlessResources::MAX_RESOURCES);

	updates.push_back(update);

	return update.slot;
}

bool BindlessResources::is_valid(BindlessHandle handle) const
{
	return handle != 0;
}

BindlessHandle BindlessResources::register_sampler(VkSampler sampler)
{
	return push_update(BINDLESS_SET_SAMPLER, sampler, VK_NULL_HANDLE);
}

BindlessHandle BindlessResources::register_sampled(VkImageView view)
{
	return push_update(BINDLESS_SET_SAMPLED, VK_NULL_HANDLE, view);
}

BindlessHandle BindlessResources::register_storage(VkImageView view)
{
	return push_update(BINDLESS_SET_STORAGE, VK_NULL_HANDLE, view);
}
