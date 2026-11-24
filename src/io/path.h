#ifndef IO_PATH_H
#define IO_PATH_H

#include "container/string.h"

namespace path {

String GetFileName(const String &path);           // Excluding extension.
String GetFileNameExt(const String &path);        // Including extension.
String GetFileExtension(const String &path);      // Including the '.'.
String GetFilePathNoExt(const String &path);
String GetFileDirectory(const String &path);      // Including the trailing '/'.

String Join(const String &path_a, const String &path_b);

// make a safe lowercase file name. surrounding whitespace
// is dropped, runs of anything but [a-z0-9_-] become a single '_'.
String SanitizeFileName(const String &name, const String &fallback);

/*
 * Simplify the file path.
 * "../foo/bar\\../asdf/../..\\" -> "../"
 */
String Normalize(const String &path);

}

#endif // IO_PATH_H
