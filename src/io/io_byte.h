#ifndef IO_BYTE_H
#define IO_BYTE_H

internal u8 IO_ByteSwap8(u8 v);
internal u16 IO_ByteSwap16(u16 v);
internal u32 IO_ByteSwap32(u32 v);
internal u64 IO_ByteSwap64(u64 v);

typedef u32 IO_ByteSerializerErrorFlags;
enum
{
	IO_ByteSerializerError_None        = 0,
	IO_ByteSerializerError_Overflowed  = 1 << 0,
	IO_ByteSerializerError_Underflowed = 1 << 1,
	IO_ByteSerializerError_FailedWrite = 1 << 2,
	IO_ByteSerializerError_FailedRead  = 1 << 3
};

typedef struct IO_ByteSerializer IO_ByteSerializer;
struct IO_ByteSerializer
{
	OS_Handle os_stream;
	u64 capacity;
	i64 at;
	IO_Endian endianness;
	IO_ByteSerializerErrorFlags errors;
};

internal u8  IO_ByteSwap8  (u8 v);
internal u16 IO_ByteSwap16 (u16 v);
internal u32 IO_ByteSwap32 (u32 v);
internal u64 IO_ByteSwap64 (u64 v);

internal IO_ByteSerializer IO_ByteStart(OS_Handle os_stream, IO_Endian endianness);
internal IO_ByteSerializer IO_ByteStartPlatformEndian(OS_Handle os_stream);

internal b32      IO_ByteOk        (const IO_ByteSerializer *s);

internal void     IO_ByteSkip      (IO_ByteSerializer *s, i64 bytes);
internal void     IO_ByteSeek      (IO_ByteSerializer *s, u64 to);
internal u64      IO_ByteTell      (IO_ByteSerializer *s);

internal void     IO_ByteWrite     (IO_ByteSerializer *s, void *bytes, u64 size);

internal void     IO_ByteReadInto  (IO_ByteSerializer *s, u64 size, void *buf);
internal u8      *IO_ByteRead      (IO_ByteSerializer *s, u64 size, Arena *arena);

internal void     IO_ByteWriteU32  (IO_ByteSerializer *s, u32 value);
internal void     IO_ByteWriteI32  (IO_ByteSerializer *s, i32 value);
internal void     IO_ByteWriteF32  (IO_ByteSerializer *s, f32 value);

internal u32      IO_ByteReadU32   (IO_ByteSerializer *s);
internal i32      IO_ByteReadI32   (IO_ByteSerializer *s);
internal f32      IO_ByteReadF32   (IO_ByteSerializer *s);

internal void     IO_ByteWriteU64  (IO_ByteSerializer *s, u64 value);
internal void     IO_ByteWriteI64  (IO_ByteSerializer *s, i64 value);
internal void     IO_ByteWriteF64  (IO_ByteSerializer *s, f64 value);

internal u64      IO_ByteReadU64   (IO_ByteSerializer *s);
internal i64      IO_ByteReadI64   (IO_ByteSerializer *s);
internal f64      IO_ByteReadF64   (IO_ByteSerializer *s);

internal void     IO_ByteWriteV2   (IO_ByteSerializer *s, v2 v);
internal void     IO_ByteWriteV3   (IO_ByteSerializer *s, v3 v);
internal void     IO_ByteWriteV4   (IO_ByteSerializer *s, v4 v);

internal v2       IO_ByteReadV2    (IO_ByteSerializer *s);
internal v3       IO_ByteReadV3    (IO_ByteSerializer *s);
internal v4       IO_ByteReadV4    (IO_ByteSerializer *s);

internal void     IO_ByteWriteStr8 (IO_ByteSerializer *s, String8 value);
internal String8  IO_ByteReadStr8  (IO_ByteSerializer *s, Arena *arena);

#define IO_ByteReadArray(s, type, count, out) IO_ByteRead((s), sizeof(type) * (count), (out))

#endif // IO_BYTE_H
