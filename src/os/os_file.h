#ifndef OS_FILE_H
#define OS_FILE_H

typedef u32 OS_FileAccess;
enum
{
	OS_FileAccess_None              = 0,
	OS_FileAccess_Read              = 1 << 0,
	OS_FileAccess_Write             = 1 << 1,
	OS_FileAccess_CreateIfMissing   = 1 << 2,
	OS_FileAccess_OverwriteIfExists = 1 << 3,
	OS_FileAccess_Append            = 1 << 4,
	OS_FileAccess_Exclusive         = 1 << 5,
	OS_FileAccess_NonBinary         = 1 << 6
};

#define OS_FILE_PRESET_OPEN       (OS_FileAccess_Write | OS_FileAccess_CreateIfMissing | OS_FileAccess_Append)
#define OS_FILE_PRESET_OPEN_RW    (OS_FileAccess_Write | OS_FileAccess_CreateIfMissing | OS_FileAccess_Append | OS_FileAccess_Read)

#define OS_FILE_PRESET_CREATE     (OS_FileAccess_Write | OS_FileAccess_CreateIfMissing | OS_FileAccess_OverwriteIfExists)
#define OS_FILE_PRESET_CREATE_RW  (OS_FileAccess_Write | OS_FileAccess_CreateIfMissing | OS_FileAccess_OverwriteIfExists | OS_FileAccess_Read)

#endif // OS_FILE_H
