#ifndef OS_FILE_SYSTEM_H
#define OS_FILE_SYSTEM_H

#include "core/types.h"

#include "container/string.h"
#include "container/vector.h"

namespace fs {

bool Delete(const String &path);
bool Exists(const String &path);

i64 GetLastWriteTime(const String &path);

bool DirectoryCreate(const String &path);
bool DirectoryDelete(const String &path);
bool DirectoryExists(const String &path);

Vector<String> ListFileNames(const String &directory); // non-recursive
Vector<String> ListFilesWithExtension(const String &directory, const String &extension); // non-recursive
Vector<String> ListDirectories(const String &directory); // non-recursive

String AbsolutePath(const String &path);
bool SetWorkingDirectory(const String &path);

bool ReadEntireFile(const String &path, Vector<u8> *out);
bool ReadEntireTextFile(const String &path, String *out);
bool WriteEntireFile(const String &path, const void *data, u64 size);
bool WriteEntireTextFile(const String &path, const String &text);

}

#endif // OS_FILE_SYSTEM_H
