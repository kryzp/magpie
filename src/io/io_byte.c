
static u8 IO_ByteSwap8(u8 v)
{
	return v;
}

static u16 IO_ByteSwap16(u16 v)
{
	return ((v & 0x00FF) << 8 |
			(v & 0xFF00) >> 8);
}

static u32 IO_ByteSwap32(u32 v)
{
	return ((v & 0x000000FF) << 24 |
			(v & 0x0000FF00) <<  8 |
			(v & 0x00FF0000) >>  8 |
			(v & 0xFF000000) >> 24);
}

static u64 IO_ByteSwap64(u64 v)
{
	return ((v & 0x00000000000000FFULL) << 56 |
			(v & 0x000000000000FF00ULL) << 40 |
			(v & 0x0000000000FF0000ULL) << 24 |
			(v & 0x00000000FF000000ULL) <<  8 |
			(v & 0x000000FF00000000ULL) >>  8 |
			(v & 0x0000FF0000000000ULL) >> 24 |
			(v & 0x00FF000000000000ULL) >> 40 |
			(v & 0xFF00000000000000ULL) >> 56);
}

static IO_ByteSerializer IO_ByteStart(OS_Handle os_stream, IO_Endian endianness)
{
	IO_ByteSerializer s = {0};
	s.os_stream = os_stream;
	s.capacity = (u64)(-1);
	s.at = 0;
	s.endianness = endianness;
	s.errors = IO_ByteSerializerError_None;

	return s;
}

static IO_ByteSerializer IO_ByteStartPlatformEndian(OS_Handle os_stream)
{
	IO_ByteSerializer s = {0};
	s.os_stream = os_stream;
	s.capacity = (u64)(-1);
	s.at = 0;
	s.endianness = IO_GetEndian();
	s.errors = IO_ByteSerializerError_None;

	return s;
}

static b32 IO_ByteOk(const IO_ByteSerializer *s)
{
	return s->errors == IO_ByteSerializerError_None;
}

static void IO_ByteSkip(IO_ByteSerializer *s, i64 bytes)
{
	if (s->at + bytes < 0)
	{
		s->at = 0;
		s->errors |= IO_ByteSerializerError_Underflowed;
	}
	
	if (s->at + bytes <= s->capacity)
	{
		s->at += bytes;
	}
	else
	{
		s->errors |= IO_ByteSerializerError_Overflowed;
	}
}

static void IO_ByteSeek(IO_ByteSerializer *s, u64 to)
{
	if (to <= s->capacity)
	{
		s->at = to;
	}
	else
	{
		s->errors |= IO_ByteSerializerError_Overflowed;
	}
}

static u64 IO_ByteTell(IO_ByteSerializer *s)
{
	return s->at;
}

static void IO_ByteWrite(IO_ByteSerializer *s, void *bytes, u64 size)
{
	if (!IO_ByteOk(s))
	{
		DebugPrintW("Byte serializer not OK: %u", s->errors);
		return;
	}

	if (s->at + size > s->capacity)
	{
		s->errors |= IO_ByteSerializerError_Overflowed;
		return;
	}

	osapi->StreamSeek(s->os_stream, s->at);

	i64 written = osapi->StreamWrite(s->os_stream, bytes, size);

	if (written != (i64)size)
	{
		s->errors |= IO_ByteSerializerError_FailedWrite;
		return;
	}

	s->at += size;
}

static void IO_ByteReadInto(IO_ByteSerializer *s, u64 size, void *buf)
{
	if (!IO_ByteOk(s))
	{
		DebugPrintW("Byte serializer not OK: %u", s->errors);
		return;
	}

	if (s->at + size > s->capacity)
	{
		s->errors |= IO_ByteSerializerError_Overflowed;
		return;
	}

	osapi->StreamSeek(s->os_stream, s->at);

	i64 read = osapi->StreamRead(s->os_stream, buf, size);

	if (read != (i64)size)
	{
		s->errors |= IO_ByteSerializerError_FailedRead;
		return;
	}

	s->at += size;
}

static u8 * IO_ByteRead(IO_ByteSerializer *s, u64 size, Arena *arena)
{
	if (!IO_ByteOk(s))
	{
		DebugPrintW("Byte serializer not OK: %u", s->errors);
		return NULL;
	}

	if (s->at + size > s->capacity)
	{
		s->errors |= IO_ByteSerializerError_Overflowed;
		return NULL;
	}

	u8 *buf = ArenaPushArray(arena, u8, size);

	osapi->StreamSeek(s->os_stream, s->at);

	i64 read = osapi->StreamRead(s->os_stream, buf, size);

	if (read != (i64)size)
	{
		s->errors |= IO_ByteSerializerError_FailedRead;
		return NULL;
	}

	s->at += size;

	return buf;
}

static void IO_ByteWriteU32(IO_ByteSerializer *s, u32 value)
{
	if (s->endianness != IO_GetEndian())
		value = IO_ByteSwap32(value);
	IO_ByteWrite(s, &value, sizeof(u32));
}

static void IO_ByteWriteI32(IO_ByteSerializer *s, i32 value)
{
	u32 raw = 0;
	MemCopy(&raw, &value, sizeof(i32));
	IO_ByteWriteU32(s, raw);
}

static void IO_ByteWriteB32(IO_ByteSerializer *s, b32 value)
{
	u32 raw = 0;
	MemCopy(&raw, &value, sizeof(b32));
	IO_ByteWriteU32(s, raw);
}

static void IO_ByteWriteF32(IO_ByteSerializer *s, f32 value)
{
	u32 raw = 0;
	MemCopy(&raw, &value, sizeof(f32));
	IO_ByteWriteU32(s, raw);
}

static u32 IO_ByteReadU32(IO_ByteSerializer *s)
{
	u32 value = 0;
	IO_ByteReadInto(s, sizeof(u32), &value);

	if (s->endianness != IO_GetEndian())
		return IO_ByteSwap32(value);
	else
		return value;
}

static i32 IO_ByteReadI32(IO_ByteSerializer *s)
{
	u32 raw = IO_ByteReadU32(s);
	i32 value;
	MemCopy(&value, &raw, sizeof(i32));
	return value;
}

static b32 IO_ByteReadB32(IO_ByteSerializer *s)
{
	u32 raw = IO_ByteReadU32(s);
	b32 value;
	MemCopy(&value, &raw, sizeof(b32));
	return value;
}

static f32 IO_ByteReadF32(IO_ByteSerializer *s)
{
	u32 raw = IO_ByteReadU32(s);
	f32 value;
	MemCopy(&value, &raw, sizeof(f32));
	return value;
}

static void IO_ByteWriteU64(IO_ByteSerializer *s, u64 value)
{
	if (s->endianness != IO_GetEndian())
		value = IO_ByteSwap64(value);
	IO_ByteWrite(s, &value, sizeof(u64));
}

static void IO_ByteWriteI64(IO_ByteSerializer *s, i64 value)
{
	u64 raw = 0;
	MemCopy(&raw, &value, sizeof(i64));
	IO_ByteWriteU64(s, raw);
}

static void IO_ByteWriteB64(IO_ByteSerializer *s, b64 value)
{
	u64 raw = 0;
	MemCopy(&raw, &value, sizeof(b64));
	IO_ByteWriteU64(s, raw);
}

static void IO_ByteWriteF64(IO_ByteSerializer *s, f64 value)
{
	u64 raw = 0;
	MemCopy(&raw, &value, sizeof(f64));
	IO_ByteWriteU64(s, raw);
}

static u64 IO_ByteReadU64(IO_ByteSerializer *s)
{
	u64 value = 0;
	IO_ByteReadInto(s, sizeof(u64), &value);

	if (s->endianness != IO_GetEndian())
		return IO_ByteSwap64(value);
	else
		return value;
}

static i64 IO_ByteReadI64(IO_ByteSerializer *s)
{
	u64 raw = IO_ByteReadU64(s);
	i64 value;
	MemCopy(&value, &raw, sizeof(i64));
	return value;
}

static b64 IO_ByteReadB64(IO_ByteSerializer *s)
{
	u64 raw = IO_ByteReadU64(s);
	b64 value;
	MemCopy(&value, &raw, sizeof(b64));
	return value;
}

static f64 IO_ByteReadF64(IO_ByteSerializer *s)
{
	u64 raw = IO_ByteReadU64(s);
	f64 value;
	MemCopy(&value, &raw, sizeof(f64));
	return value;
}

static void IO_ByteWriteV2(IO_ByteSerializer *s, v2 v)
{
	IO_ByteWriteF32(s, v.x);
	IO_ByteWriteF32(s, v.y);
}

static void IO_ByteWriteV3(IO_ByteSerializer *s, v3 v)
{
	IO_ByteWriteF32(s, v.x);
	IO_ByteWriteF32(s, v.y);
	IO_ByteWriteF32(s, v.z);
}

static void IO_ByteWriteV4(IO_ByteSerializer *s, v4 v)
{
	IO_ByteWriteF32(s, v.x);
	IO_ByteWriteF32(s, v.y);
	IO_ByteWriteF32(s, v.z);
	IO_ByteWriteF32(s, v.w);
}

static v2 IO_ByteReadV2(IO_ByteSerializer *s)
{
	v2 value = {0};
	value.x = IO_ByteReadF32(s);
	value.y = IO_ByteReadF32(s);
	return value;
}

static v3 IO_ByteReadV3(IO_ByteSerializer *s)
{
	v3 value = {0};
	value.x = IO_ByteReadF32(s);
	value.y = IO_ByteReadF32(s);
	value.z = IO_ByteReadF32(s);
	return value;
}

static v4 IO_ByteReadV4(IO_ByteSerializer *s)
{
	v4 value = {0};
	value.x = IO_ByteReadF32(s);
	value.y = IO_ByteReadF32(s);
	value.z = IO_ByteReadF32(s);
	value.w = IO_ByteReadF32(s);
	return value;
}

static void IO_ByteWriteStr8(IO_ByteSerializer *s, String8 value)
{
	IO_ByteWriteU64(s, value.len);
	IO_ByteWrite(s, value.str, value.len);
}

static String8 IO_ByteReadStr8(IO_ByteSerializer *s, Arena *arena)
{
	u64 len = IO_ByteReadU64(s);
	String8 value = String8Alloc(arena, len);
	IO_ByteReadInto(s, value.len, value.str);
	return value;
}
