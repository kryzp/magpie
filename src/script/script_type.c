
internal SCR_Argument
SCR_ArgumentF32(f32 v)
{
	SCR_Argument arg = {0};
	arg.type = SCR_ArgType_F32;
	arg.float32 = v;

	return arg;
}

internal SCR_Argument
SCR_ArgumentI32(i32 v)
{
	SCR_Argument arg = {0};
	arg.type = SCR_ArgType_I32;
	arg.int32 = v;

	return arg;
}

internal SCR_Argument
SCR_ArgumentB32(b32 v)
{
	SCR_Argument arg = {0};
	arg.type = SCR_ArgType_B32;
	arg.bool32 = v;

	return arg;
}

internal SCR_Argument
SCR_ArgumentStr(String8 v)
{
	SCR_Argument arg = {0};
	arg.type = SCR_ArgType_String8;
	arg.string8 = v;

	return arg;
}

internal SCR_Argument
SCR_ArgumentTaggedU32(u32 v, u32 tag)
{
	SCR_Argument arg = {0};
	arg.type = SCR_ArgType_TaggedU32;
	arg.tagged32.unsigned32 = v;
	arg.tagged32.tag = tag;

	return arg;
}
