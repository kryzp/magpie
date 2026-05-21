#ifndef SCRIPT_TYPE_H
#define SCRIPT_TYPE_H

/*
typedef enum SCR_TypeTag
{
	SCR_TypeTag_None,
	SCR_TypeTag_EntityUID,
	SCR_TypeTag_AssetHandle,
	SCR_TypeTag_CameraHandle,
	SCR_TypeTag_COUNT
}
SCR_TypeTag;
*/

/*
typedef struct SCR_TypeTag SCR_TypeTag;
struct SCR_TypeTag
{
	String8 name;
	u32 value;
};
*/

// todo: hjw do we wanna do tags

typedef enum SCR_ArgType
{
	SCR_ArgType_Nil = 0,
	SCR_ArgType_F32,
	SCR_ArgType_I32,
	SCR_ArgType_Bool,
	SCR_ArgType_String,
	SCR_ArgType_TaggedU32,
	SCR_ArgType_COUNT,
}
SCR_ArgType;

typedef struct SCR_Argument SCR_Argument;
struct SCR_Argument
{
	SCR_ArgType type;

	union
	{
		f32 float32;
		i32 int32;
		b32 bool32;
		String8 string8;
		
		struct
		{
			u32 unsigned32;
			u32 tag;
		}
		tagged32;
	};
};

#endif // SCRIPT_TYPE_H
