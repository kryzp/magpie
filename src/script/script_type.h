#ifndef SCRIPT_TYPE_H
#define SCRIPT_TYPE_H

typedef enum S_ArgType
{
	S_ArgType_Nil = 0,
	S_ArgType_F32,
	S_ArgType_I32,
	S_ArgType_B32,
	S_ArgType_String8,
	S_ArgType_TaggedU32,
	S_ArgType_COUNT,
}
S_ArgType;

typedef struct S_Argument S_Argument;
struct S_Argument
{
	S_ArgType type;

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

static S_Argument S_ArgF32(f32 v);
static S_Argument S_ArgI32(i32 v);
static S_Argument S_ArgB32(b32 v);
static S_Argument S_ArgStr(String8 v);
static S_Argument S_ArgTaggedU32(u32 v, u32 tag);

static inline u64 S_PackTaggedU32(u32 value, u32 tag);
static inline void S_UnpackTaggedU32(u64 packed, u32 *out_value, u32 *out_tag);

#endif // SCRIPT_TYPE_H
