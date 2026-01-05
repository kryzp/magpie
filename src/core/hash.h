#pragma once

#include "types.h"

namespace hash
{

u64 generic(const void *key, u64 length);
u64 generic_combine(u64 start, const void *key, u64 length);
u64 cstr(const char *string);

}
