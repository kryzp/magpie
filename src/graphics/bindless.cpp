#include "bindless.h"

using namespace gfx;

BindlessResources::BindlessResources()
	: pool()
	, layouts{}
	, sets{}
	, samplers(0)
	, views(0)
	, updates()
{
}

BindlessResources::~BindlessResources()
{
}

void BindlessResources::push_update(BindlessSetKind kind, BindlessHandle handle, VkSampler sampler, VkImageView view)
{
	assert(handle < BindlessResources::MAX_RESOURCES);

	BindlessUpdate update = {};
	update.kind = kind;
	update.sampler = sampler;
	update.view = view;
	update.slot = handle;
	
	updates.push_back(update);
}

bool BindlessResources::is_valid(BindlessHandle handle) const
{
	return handle != 0;
}

BindlessHandle BindlessResources::register_sampler(VkSampler sampler)
{
	BindlessHandle handle = ++samplers;
	update_sampler(handle, sampler);
	return handle;
}

BindlessHandle BindlessResources::register_view(VkImageView view, bool storage)
{
	BindlessHandle handle = ++views;
	update_view(handle, view, storage);
	return handle;
}

void BindlessResources::update_sampler(BindlessHandle handle, VkSampler sampler)
{
	push_update(BINDLESS_SET_SAMPLER, handle, sampler, VK_NULL_HANDLE);
}

void BindlessResources::update_view(BindlessHandle handle, VkImageView view, bool storage)
{
	push_update(BINDLESS_SET_SAMPLED, handle, VK_NULL_HANDLE, view);

	if (storage)
		push_update(BINDLESS_SET_STORAGE, handle, VK_NULL_HANDLE, view);
}
