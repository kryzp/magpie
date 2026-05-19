#ifndef IO_ENDIAN_H
#define IO_ENDIAN_H

typedef enum IO_Endian
{
	IO_Endian_Little,
	IO_Endian_Big,
	IO_Endian_COUNT
}
IO_Endian;

internal b32 IO_IsLittleEndian(void);
internal b32 IO_IsBigEndian(void);

internal IO_Endian IO_GetEndian(void);

#endif // IO_ENDIAN_H
