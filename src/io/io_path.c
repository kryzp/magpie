
internal String8
IO_PathGetFileName(Arena *arena, String8 path)
{
	// TODO
	
	AssertTrue(false);

	return (String8) {0};
}

internal String8
IO_PathGetFileNameExt(Arena *arena, String8 path)
{
	u64 index = 0;
	
	for (u64 i = 0; i < path.len; i++)
	{
		if (path.str[i] == '/' || path.str[i] == '\\')
			index = i + 1;
	}

	String8 out = String8Alloc(arena, path.len - index);
	MemCopy(out.str, path.str + index, out.len);

	return out;
}

internal String8
IO_PathGetFileExtension(Arena *arena, String8 path)
{
	u32 index = String8FindLastIncl(path, String8Lit("."));

	String8 out = String8Alloc(arena, path.len - index);

	MemCopy(out.str, path.str + index, out.len);

	return out;
}

internal String8
IO_PathGetFilePathNoExt(Arena *arena, String8 path)
{
	// TODO

	AssertTrue(false);

	return (String8) {0};
}

internal String8
IO_PathGetFileDirectory(Arena *arena, String8 path)
{
	// TODO

	AssertTrue(false);

	return (String8) {0};
}

internal String8
IO_PathJoin(Arena *arena, String8 path_a, String8 path_b)
{
	if (path_a.len <= 0)
		return IO_PathNormalize(arena, path_b);

	if (path_b.len <= 0)
		return IO_PathNormalize(arena, path_a);

	ScratchArena scratch = ScratchBegin(&arena, 1);

	String8 joined = String8Append(scratch.arena, String8Append(scratch.arena, path_a, String8Lit("/")), path_b);
	
	String8 result = IO_PathNormalize(arena, joined);

	ScratchRelease(&scratch);
	
	return result;
}

internal String8
IO_PathNormalize(Arena *arena, String8 path)
{
	// TODO

	AssertTrue(false);

	return (String8) {0};
}
