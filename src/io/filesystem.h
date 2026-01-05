#pragma once

#include "container/string.h"

namespace io
{

namespace path
{

String get_file_name(const String &path);
String get_file_extension(const String &path);
String get_file_directory(const String &path);

String join(const String &path_a, String path_b);

/*
 * "Simplifies" a file path.
 * ../foo/bar\\../asdf/../..\\ -> ../
 */
String normalize(const String &path);

}

}
