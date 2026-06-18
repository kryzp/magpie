#ifndef SCRIPT_TYPE_H
#define SCRIPT_TYPE_H

typedef enum SCR_ArgType
{
	SCR_ArgType_Nil = 0,
	SCR_ArgType_F32,
	SCR_ArgType_I32,
	SCR_ArgType_B32,
	SCR_ArgType_String8,
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

		// ok so basically, to clarify the type being passed
		// since lua takes in 64-bit integers, we can split it
		// up into two seperate 32-bit integers, with the other
		// half being used as a tag, this way we get an error
		// rather than a silent failure if e.g: we pass an audio
		// handle instead of an entity handle into a function!
		// TODO: maybe change this later? feels hacky.
		struct
		{
			u32 unsigned32;
			u32 tag;
		}
		tagged32;
	};
};

internal SCR_Argument SCR_ArgumentF32(f32 v);
internal SCR_Argument SCR_ArgumentI32(i32 v);
internal SCR_Argument SCR_ArgumentB32(b32 v);
internal SCR_Argument SCR_ArgumentStr(String8 v);
internal SCR_Argument SCR_ArgumentTaggedU32(u32 v, u32 tag);

#endif // SCRIPT_TYPE_H
