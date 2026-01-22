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

	struct BindlessSampler {
		BindlessHandle sampler;
	};

	struct BindlessView {
		BindlessHandle sampled;
		BindlessHandle storage;
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
		constexpr static u32 MAX_RESOURCES = 256;

		BindlessResources();
		~BindlessResources();

		bool is_valid(BindlessHandle handle) const;

		BindlessSampler register_sampler(VkSampler sampler);
		BindlessView register_view(VkImageView view, bool is_sampled, bool is_storage);

		const VkDescriptorSetLayout *get_layouts() const
		{
			return layouts;
		}

		const VkDescriptorSet *get_sets() const
		{
			return sets;
		}

	private:
		BindlessHandle push_update(BindlessSetKind kind, VkSampler sampler, VkImageView view);

		VkDescriptorPool pool;
		VkDescriptorSetLayout layouts[BINDLESS_SET_MAX_ENUM];
		VkDescriptorSet sets[BINDLESS_SET_MAX_ENUM];

		u32 resource_counts[BINDLESS_SET_MAX_ENUM];

		Vector<BindlessUpdate> updates;
	};
}
