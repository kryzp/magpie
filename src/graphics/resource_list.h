#ifndef GRAPHICS_RESOURCE_LIST_H
#define GRAPHICS_RESOURCE_LIST_H

#include "core/types.h"
#include "graphics/resource_key.h"
#include "os/spin_lock.h"
#include "container/hash_map.h"

namespace gfx {

template <typename T>
class ResourceList {
public:
	ResourceList()
		: next_auto_key(1) // 0 is the null key.
	{
	}

	ResourceList(const ResourceList &) = delete;
	ResourceList &operator=(const ResourceList &) = delete;

	ResourceKey Push(const T &resource, ResourceKey key)
	{
		SpinLockScope scope(lock);
		resources[key.value] = resource;
		return key;
	}

	ResourceKey PushAuto(const T &resource)
	{
		SpinLockScope scope(lock);

		ResourceKey key(next_auto_key++);
		resources[key.value] = resource;

		return key;
	}

	T *Get(ResourceKey key)
	{
		SpinLockScope scope(lock);

		auto it = resources.find(key.value);

		if (it == resources.end())
			return nullptr;

		return &it->second;
	}

	void Return(ResourceKey key)
	{
		SpinLockScope scope(lock);
		resources.erase(key.value);
	}

	template <typename Fn>
	void ForEachNotLocked(Fn fn)
	{
		for (auto &pair : resources)
			fn(pair.second);
	}

private:
	HashMap<u64, T> resources;
	u64 next_auto_key;
	SpinLock lock;
};
}

#endif // GRAPHICS_RESOURCE_LIST_H
