#ifndef CORE_POOL_H
#define CORE_POOL_H

#include "core/types.h"

#include "container/vector.h"

class SlotPool {
public:
	SlotPool() = default;
	explicit SlotPool(unsigned capacity);

	bool Alloc(unsigned *out_index);
	void Free(unsigned index);

	[[nodiscard]] unsigned LiveCount() const;

	[[nodiscard]] unsigned Capacity() const
	{
		return capacity;
	}

private:
	Vector<unsigned> free_indices;
	unsigned capacity = 0;
};

class DensePool {
public:
	DensePool() = default;
	explicit DensePool(unsigned capacity);

	unsigned GetStableID();

	// USAGE:
	//   unsigned dense = pool.IndexFromID(id);
	//   unsigned moved = pool.FreeID(id);
	//   data[dense] = data[moved]; // <-- done outside of the pool code, must be done manually!
	unsigned FreeID(unsigned id);

	[[nodiscard]] unsigned IndexFromID(unsigned id) const;
	[[nodiscard]] unsigned IDFromIndex(unsigned index) const;

	[[nodiscard]] unsigned LiveCount() const;

	[[nodiscard]] unsigned Capacity() const
	{
		return capacity;
	}

private:
	Vector<unsigned> id_to_dense;
	Vector<unsigned> dense_to_id;
	Vector<unsigned> free_ids;

	unsigned count = 0;
	unsigned capacity = 0;
};

#endif // CORE_POOL_H
