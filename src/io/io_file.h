#ifndef IO_FILE_H
#define IO_FILE_H

typedef struct IO_ByteSpan IO_ByteSpan;
struct IO_ByteSpan
{
	const u8 *bytes;
	u64 size;
};

static IO_ByteSpan IO_ReadEntireFile(Arena *arena, String8 path);

#endif // IO_FILE_H
