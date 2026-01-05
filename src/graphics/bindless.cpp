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

	updates.push_back(update);

	return update.slot;
}

bool BindlessResources::is_valid(BindlessHandle handle) const
{
	return handle != 0;
}

BindlessSampler BindlessResources::register_sampler(VkSampler sampler)
{
	BindlessSampler bindless_sampler = {};

	bindless_sampler.sampler = push_update(BINDLESS_SET_SAMPLER, sampler, VK_NULL_HANDLE);

	return bindless_sampler;
}

BindlessView BindlessResources::register_view(VkImageView view, bool is_sampled, bool is_storage)
{
	BindlessView bindless_view = {};

	if (is_sampled) bindless_view.sampled = push_update(BINDLESS_SET_SAMPLED, VK_NULL_HANDLE, view);
	if (is_storage) bindless_view.storage = push_update(BINDLESS_SET_STORAGE, VK_NULL_HANDLE, view);

	return bindless_view;
}
