#ifndef IO_PATH
#define IO_PATH

internal String8 IO_PathGetFileName(Arena *arena, String8 path);
internal String8 IO_PathGetFileExtension(Arena *arena, String8 path);
internal String8 IO_PathGetFileDirectory(Arena *arena, String8 path);

internal String8 IO_PathJoin(Arena *arena, String8 path_a, String8 path_b);

/*
 * Simplify the file path.
 * "../foo/bar\\../asdf/../..\\" -> "../"
 */
internal String8 IO_PathNormalize(Arena *arena, String8 path);

#endif // IO_PATH
