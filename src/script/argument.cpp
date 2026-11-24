#include "script/argument.h"

scr::Argument scr::Argument::Float(float v)
{
	Argument arg;
	arg.type = ArgType_Float;
	arg.float_value = v;
	return arg;
}

scr::Argument scr::Argument::Int(int v)
{
	Argument arg;
	arg.type = ArgType_Int;
	arg.int_value = v;
	return arg;
}

scr::Argument scr::Argument::Bool(bool v)
{
	Argument arg;
	arg.type = ArgType_Bool;
	arg.bool_value = v;
	return arg;
}

scr::Argument scr::Argument::Str(const String &v)
{
	Argument arg;
	arg.type = ArgType_String;
	arg.string_value = v;
	return arg;
}

scr::Argument scr::Argument::TaggedU32(unsigned v, unsigned tag)
{
	Argument arg;
	arg.type = ArgType_TaggedU32;
	arg.tagged_value = v;
	arg.tag = tag;
	return arg;
}
