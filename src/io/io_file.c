
internal IO_ByteSpan
IO_ReadEntireFile(Arena *arena, String8 path)
{
	IO_ByteSpan source = {0};

	OS_Handle stream = osapi->StreamFromFile(path, OS_FileAccess_Read);
	
	if (OS_HandleIsNull(stream))
	{
		DebugPrintE("Cannot open \"%.*s\".", String8VArg(path));
		goto end;
	}

	i64 size = osapi->StreamSize(stream);
	u8 *buf  = ArenaPushArray(arena, u8, size);
	i64 read = osapi->StreamRead(stream, buf, size);
	osapi->StreamClose(stream);

	if (read != size)
	{
		DebugPrintE("Short read on \"%.*s\" (%lld of %llu).", String8VArg(path), read, size);
		goto end;
	}

	source.bytes = buf;
	source.size = size;

end:
	return source;
}
