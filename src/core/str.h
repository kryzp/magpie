#ifndef CORE_STR_H
#define CORE_STR_H

#include "container/string.h"

namespace str {

String ToLower(const String &value);

String Trim(const String &value);

bool StartsWith(const String &value, const String &prefix);
bool EndsWith(const String &value, const String &suffix);

}

#endif // CORE_STR_H
