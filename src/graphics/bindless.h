#pragma once

#include <volk/volk.h>

#include "core/types.h"

#include "container/vector.h"

namespace gfx
{
	using BindlessHandle = u32;

	enum BindlessSetKind {
		BINDLESS_SET_SAMPLER,
		BINDLESS_SET_SAMPLED,
		BINDLESS_SET_STORAGE,
		BINDLESS_SET_MAX_ENUM
	};

	class BindlessResources {
		friend class Device;

		struct BindlessUpdate {
			BindlessSetKind kind;
			BindlessHandle slot;
			VkSampler sampler;
			VkImageView view;
		};

	public:
		constexpr static u32 MAX_RESOURCES = 16384; // TODO: Get this from physical properties.

		BindlessResources();
		~BindlessResources();

		bool is_valid(BindlessHandle handle) const;

		BindlessHandle register_sampler(VkSampler sampler);
		BindlessHandle register_view(VkImageView view, bool storage);

		void update_sampler(BindlessHandle handle, VkSampler sampler);
		void update_view(BindlessHandle handle, VkImageView, bool storage);

		void free_sampler(BindlessHandle handle);
		void free_view(BindlessHandle handle);

		const VkDescriptorSetLayout *get_layouts() const
		{
			return layouts;
		}

		const VkDescriptorSet *get_sets() const
		{
			return sets;
		}

	private:
		void push_update(BindlessSetKind kind, BindlessHandle handle, VkSampler sampler, VkImageView view);

		VkDescriptorPool pool;
		VkDescriptorSetLayout layouts[BINDLESS_SET_MAX_ENUM];
		VkDescriptorSet sets[BINDLESS_SET_MAX_ENUM];

		u32 samplers;
		u32 views;

		Vector<BindlessHandle> free_samplers;
		Vector<BindlessHandle> free_views;

		Vector<BindlessUpdate> updates;
	};
}
