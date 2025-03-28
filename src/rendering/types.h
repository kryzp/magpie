#ifndef RENDERING_TYPES_H_
#define RENDERING_TYPES_H_

#include "math/calc.h"

namespace llt
{
	using BindlessResourceHandle = int;
	constexpr BindlessResourceHandle BINDLESS_RESOURCE_HANDLE_INVALID = Calc<int>::maxValue();
}

#endif // RENDERING_TYPES_H_
