#include "core/pool.h"
#include "core/log.h"

SlotPool::SlotPool(unsigned capacity_)
	: capacity(capacity_)
{
	free_indices.reserve(capacity);

	for (int i = 0; i < capacity; i++)
		free_indices.push_back(capacity - 1 - i);
}

bool SlotPool::Alloc(unsigned *out_index)
{
	if (free_indices.empty())
		return false;

	*out_index = free_indices.back();
	free_indices.pop_back();

	return true;
}

void SlotPool::Free(unsigned index)
{
	AssertTrue(free_indices.size() < capacity);
	free_indices.push_back(index);
}

unsigned SlotPool::LiveCount() const
{
	return capacity - free_indices.size();
}

DensePool::DensePool(unsigned capacity_)
	: id_to_dense(capacity_)
	, dense_to_id(capacity_)
	, count(0)
	, capacity(capacity_)
{
	free_ids.reserve(capacity);

	for (int i = 0; i < capacity; i++)
		free_ids.push_back(capacity - 1 - i);
}

unsigned DensePool::GetStableID()
{
	AssertTrue(!free_ids.empty());

	unsigned id = free_ids.back();
	free_ids.pop_back();

	unsigned dense = count++;

	id_to_dense[id] = dense;
	dense_to_id[dense] = id;

	return id;
}

unsigned DensePool::FreeID(unsigned id)
{
	unsigned dense = id_to_dense[id];
	unsigned last_dense = --count;
	unsigned last_id = dense_to_id[last_dense];

	dense_to_id[dense] = last_id;
	id_to_dense[last_id] = dense;

	free_ids.push_back(id);

	return last_dense;
}

unsigned DensePool::IndexFromID(unsigned id) const
{
	return id_to_dense[id];
}

unsigned DensePool::IDFromIndex(unsigned index) const
{
	return dense_to_id[index];
}

unsigned DensePool::LiveCount() const
{
	return count;
}
