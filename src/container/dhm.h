#pragma once

#include "core/types.h"

#include "vector.h"

/*
 * This is a little confusing so for later reference:
 *
 * --> We need tight memory packing for the GPU upload but we need
 *     stable pointers at the same time.
 *
 *     So basically, we store an extra level of indirection, which
 *     we call a handle entry.
 *
 *     It maps from the "user" handle (RenderHandle)
 *     to the actual internal index of the resource.
 *
 *         RenderHandle (index, generation)
 *      => HandleEntry[index] (dense_index, generation)
 *      => ObjectData[dense_index] (gpu data)
 *
 *     The generation parameter is used to make sure the handle
 *     hasn't gone stale.
 * 
 * <Handle> must expose two variables u32 index, u32 generation.
 */
template <typename T, typename Handle>
struct DenseHandleMap {
	struct DenseHandle {
		u32 dense_index;
		u32 generation;
	};

	Vector<DenseHandle> handles;
	Vector<u32> free_indices;
	Vector<Handle> back_references;
	Vector<T> data;

	DenseHandleMap()
		: handles()
		, free_indices()
		, back_references()
		, data()
	{
	}

	Handle insert(const T &value)
	{
		Handle handle = {};
		handle.index = alloc_handle_index();
		handle.generation = handles[handle.index].generation;

		u32 dense_index = data.size();

		data.push_back(value);
		back_references.push_back(handle);

		handles[handle.index].dense_index = dense_index;

		return handle;
	}

	void remove(Handle handle)
	{
		if (!is_valid(handle))
			return;

		DenseHandle &entry = handles[handle.index];

		u32 curr_dense_index = entry.dense_index;
		u32 prev_dense_index = data.size() - 1;

		if (curr_dense_index != prev_dense_index) {

			// Previous slot user is now the current slot.
			Handle prev_handle = back_references[prev_dense_index];

			data[curr_dense_index] = data[prev_dense_index];
			back_references[curr_dense_index] = back_references[prev_dense_index];

			handles[prev_handle.index].dense_index = curr_dense_index;
		}

		data.pop_back();
		back_references.pop_back();
		free_indices.push_back(handle.index);
	}

	T &get(Handle handle)
	{
		assert(is_valid(handle));

		u32 dense_index = handles[handle.index].dense_index;
		return data[dense_index];
	}

	const T &get(Handle handle) const
	{
		assert(is_valid(handle));

		u32 dense_index = handles[handle.index].dense_index;
		return data[dense_index];
	}

	bool is_valid(Handle handle) const
	{
		return
			handle.index < handles.size() &&
			handle.generation == handles[handle.index].generation;
	}

private:
	u32 alloc_handle_index()
	{
		u32 index = 0;

		if (!free_indices.empty()) {
			index = free_indices.back();
			free_indices.pop_back();
			handles[index].generation++;
		} else {
			index = handles.size();
			handles.push_back({ 0, 1 });
		}

		return index;
	}
};
