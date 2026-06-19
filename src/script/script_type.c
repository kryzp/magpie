
internal S_Argument
S_ArgF32(f32 v)
{
	S_Argument arg = {0};
	arg.type = S_ArgType_F32;
	arg.float32 = v;

	return arg;
}

internal S_Argument
S_ArgI32(i32 v)
{
	S_Argument arg = {0};
	arg.type = S_ArgType_I32;
	arg.int32 = v;

	return arg;
}

internal S_Argument
S_ArgB32(b32 v)
{
	S_Argument arg = {0};
	arg.type = S_ArgType_B32;
	arg.bool32 = v;

	return arg;
}

internal S_Argument
S_ArgStr(String8 v)
{
	S_Argument arg = {0};
	arg.type = S_ArgType_String8;
	arg.string8 = v;

	return arg;
}

internal S_Argument
S_ArgTaggedU32(u32 v, u32 tag)
{
	S_Argument arg = {0};
	arg.type = S_ArgType_TaggedU32;
	arg.tagged32.unsigned32 = v;
	arg.tagged32.tag = tag;

	return arg;
}

internal inline u64
S_PackTaggedU32(u32 value, u32 tag)
{
	return ((u64)tag << 32) | (u64)value;
}

internal inline void
S_UnpackTaggedU32(u64 packed, u32 *out_value, u32 *out_tag)
{
	*out_tag   = (u32)(packed >> 32);
	*out_value = (u32)(packed & 0xFFFFFFFFu);
}
