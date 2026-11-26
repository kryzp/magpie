#ifndef IO_JSON_H
#define IO_JSON_H

#include "core/types.h"
#include "container/string.h"
#include "container/vector.h"

namespace json {

enum Type {
	Type_Null,
	Type_Bool,
	Type_Int,
	Type_Double,
	Type_String,
	Type_Array,
	Type_Object
};

class Value {
public:
	Value()
		: type(Type_Null)
		, bool_value(false)
		, int_value(0)
		, double_value(0.0)
	{
	}

	static Value MakeNull()
	{
		return Value();
	}

	static Value MakeBool(bool v)
	{
		Value r;
		r.type = Type_Bool;
		r.bool_value = v;
		return r;
	}

	static Value MakeInt(i64 v)
	{
		Value r;
		r.type = Type_Int;
		r.int_value = v;
		r.double_value = static_cast<double>(v);
		return r;
	}

	static Value MakeDouble(double v)
	{
		Value r;
		r.type = Type_Double;
		r.double_value = v;
		r.int_value = static_cast<i64>(v);
		return r;
	}

	static Value MakeString(const String &v)
	{
		Value r;
		r.type = Type_String;
		r.string_value = v;
		return r;
	}

	static Value MakeArray()
	{
		Value r;
		r.type = Type_Array;
		return r;
	}

	static Value MakeObject()
	{
		Value r;
		r.type = Type_Object;
		return r;
	}

	Type GetType() const
	{
		return type;
	}

	bool IsNull() const
	{
		return type == Type_Null;
	}

	bool IsBool() const
	{
		return type == Type_Bool;
	}

	bool IsNumber() const
	{
		return type == Type_Int || type == Type_Double;
	}

	bool IsString() const
	{
		return type == Type_String;
	}

	bool IsArray() const
	{
		return type == Type_Array;
	}

	bool IsObject() const
	{
		return type == Type_Object;
	}

	bool AsBool() const;
	i64 AsI64() const;

	i32 AsInt() const
	{
		return static_cast<i32>(AsI64());
	}

	double AsDouble() const;

	float AsFloat() const
	{
		return static_cast<float>(AsDouble());
	}

	String AsString() const;

	const String &Name() const
	{
		return name;
	}

	// Arrays and objects.
	u32 Size() const
	{
		return static_cast<u32>(children.size());
	}

	const Value &At(u32 index) const
	{
		return children[index];
	}

	Value &At(u32 index)
	{
		return children[index];
	}

	const Vector<Value> &Children() const
	{
		return children;
	}

	const Value *Get(const String &key) const;
	Value *Get(const String &key);

	bool Has(const String &key) const
	{
		return Get(key) != nullptr;
	}

	String GetString(const String &key, const String &fallback) const;
	i32 GetInt(const String &key, i32 fallback) const;
	i64 GetLong(const String &key, i64 fallback) const;
	float GetFloat(const String &key, float fallback) const;
	bool GetBool(const String &key, bool fallback) const;

	Value &Set(const String &key, const Value &value);
	Value &Push(const Value &value);

	void Erase(u32 index);
	void SwapChildren(u32 a, u32 b);

	String Write() const;
	String WritePretty(u32 single_line_columns) const;

private:
	void WriteCompact(String *out) const;
	void WritePrettyInto(String *out, u32 indent, u32 single_line_columns) const;
	void WriteScalar(String *out) const;

	Type type;
	String name;

	bool bool_value;
	i64 int_value;
	double double_value;
	String string_value;

	Vector<Value> children;
};

bool Parse(const String &text, Value *out, String *error);

String Quote(const String &str);

}

#endif // IO_JSON_H
