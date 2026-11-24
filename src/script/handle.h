#ifndef SCRIPT_HANDLE_H
#define SCRIPT_HANDLE_H

#include "core/types.h"

namespace scr {

struct CoroutineHandle {
	unsigned index = 0;
	unsigned generation = 0;

	bool IsNull() const
	{
		return index == 0 && generation == 0;
	}

	bool operator==(const CoroutineHandle &other) const
	{
		return index == other.index && generation == other.generation;
	}

	bool operator!=(const CoroutineHandle &other) const
	{
		return !(*this == other);
	}
};

struct Ref {
	int value = -1;

	bool IsNull() const
	{
		return value < 0;
	}

	bool operator==(const Ref &other) const
	{
		return value == other.value;
	}

	bool operator!=(const Ref &other) const
	{
		return value != other.value;
	}
};

}

#endif // SCRIPT_HANDLE_H
