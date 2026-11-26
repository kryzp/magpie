#ifndef SCRIPT_ARGUMENT_H
#define SCRIPT_ARGUMENT_H

#include "core/types.h"

#include "container/string.h"

namespace scr {

enum ArgType {
	ArgType_Nil = 0,
	ArgType_Float,
	ArgType_Int,
	ArgType_Bool,
	ArgType_String,
	ArgType_TaggedU32,
	ArgType_MaxEnum
};

struct Argument {
	ArgType type = ArgType_Nil;

	// todo: use union (convert String to const char * + length)
	float float_value = 0.f;
	int int_value = 0;
	bool bool_value = false;
	String string_value;

	// ok so basically, to clarify the type being passed
	// since lua takes in 64-bit integers, we can split it
	// up into two seperate 32-bit integers, with the other
	// half being used as a tag, this way we get an error
	// rather than a silent failure if e.g: we pass an audio
	// handle instead of an entity handle into a function!
	// TODO: maybe change this later? feels hacky.
	unsigned tagged_value = 0;
	unsigned tag = 0;

	static Argument Float(float v);
	static Argument Int(int v);
	static Argument Bool(bool v);
	static Argument Str(const String &v);
	static Argument TaggedU32(unsigned v, unsigned tag);
};

inline u64 PackTaggedU32(unsigned value, unsigned tag)
{
	return
		(static_cast<u64>(tag) << 32) |
		(static_cast<u64>(value) << 0);
}

inline void UnpackTaggedU32(u64 packed, unsigned *out_value, unsigned *out_tag)
{
	*out_tag = static_cast<unsigned>(packed >> 32);
	*out_value = static_cast<unsigned>(packed & 0xFFFFFFFFu);
}
}

#endif // SCRIPT_ARGUMENT_H
