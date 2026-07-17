#ifndef IO_PATH
#define IO_PATH

static String8 IO_PathGetFileName(Arena *arena, String8 path);    // Excluding extension.
static String8 IO_PathGetFileNameExt(Arena *arena, String8 path); // Including extension.
static String8 IO_PathGetFileExtension(Arena *arena, String8 path);
static String8 IO_PathGetFilePathNoExt(Arena *arena, String8 path);
static String8 IO_PathGetFileDirectory(Arena *arena, String8 path);

static String8 IO_PathJoin(Arena *arena, String8 path_a, String8 path_b);

/*
 * Simplify the file path.
 * "../foo/bar\\../asdf/../..\\" -> "../"
 */
static String8 IO_PathNormalize(Arena *arena, String8 path);

#endif // IO_PATH
