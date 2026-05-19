
internal b32
IO_IsLittleEndian(void)
{
	u16 x = 1;
	return *(u8 *)&x == 1;
}

internal b32
IO_IsBigEndian(void)
{
	u16 x = 1;
	return *(u8 *)&x != 1;
}

internal IO_Endian
IO_GetEndian(void)
{
	if (IO_IsLittleEndian())
		return IO_Endian_Little;

	if (IO_IsBigEndian())
		return IO_Endian_Big;

	AssertTrue(false);
	
	return IO_Endian_COUNT;
}
