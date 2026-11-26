#include "io/json.h"

#include <utility>

#include <stdio.h>
#include <stdlib.h>
#include <cmath>
#include <string.h>

bool json::Value::AsBool() const
{
	switch (type) {
		case Type_Bool:
			return bool_value;
		case Type_Int:
			return int_value != 0;
		case Type_Double:
			return double_value != 0.0;
		case Type_String:
			return string_value == "true";
		default:
			return false;
	}
}

i64 json::Value::AsI64() const
{
	switch (type) {
		case Type_Bool:
			return bool_value ? 1 : 0;
		case Type_Int:
			return int_value;
		case Type_Double:
			return static_cast<i64>(double_value);
		case Type_String:
			return strtoll(string_value.c_str(), nullptr, 10);
		default:
			return 0;
	}
}

double json::Value::AsDouble() const
{
	switch (type) {
		case Type_Bool:
			return bool_value ? 1.0 : 0.0;
		case Type_Int:
			return static_cast<double>(int_value);
		case Type_Double:
			return double_value;
		case Type_String:
			return strtod(string_value.c_str(), nullptr);
		default:
			return 0.0;
	}
}

String json::Value::AsString() const
{
	switch (type) {
		case Type_String:
			return string_value;
		case Type_Bool:
			return bool_value ? "true" : "false";
		case Type_Null:
			return "null";
		case Type_Int:
		case Type_Double: {
			String out;
			WriteScalar(&out);
			return out;
		}
		default:
			return String();
	}
}

const json::Value *json::Value::Get(const String &key) const
{
	for (const Value &child : children) {
		if (child.name == key)
			return &child;
	}

	return nullptr;
}

json::Value *json::Value::Get(const String &key)
{
	for (Value &child : children) {
		if (child.name == key)
			return &child;
	}

	return nullptr;
}

String json::Value::GetString(const String &key, const String &fallback) const
{
	const Value *v = Get(key);

	if (!v || v->IsNull())
		return fallback;

	return v->AsString();
}

i32 json::Value::GetInt(const String &key, i32 fallback) const
{
	const Value *v = Get(key);

	if (!v || v->IsNull())
		return fallback;

	return v->AsInt();
}

i64 json::Value::GetLong(const String &key, i64 fallback) const
{
	const Value *v = Get(key);

	if (!v || v->IsNull())
		return fallback;

	return v->AsI64();
}

float json::Value::GetFloat(const String &key, float fallback) const
{
	const Value *v = Get(key);

	if (!v || v->IsNull())
		return fallback;

	return v->AsFloat();
}

bool json::Value::GetBool(const String &key, bool fallback) const
{
	const Value *v = Get(key);

	if (!v || v->IsNull())
		return fallback;

	return v->AsBool();
}

json::Value &json::Value::Set(const String &key, const Value &value)
{
	if (type != Type_Object) {
		type = Type_Object;
		children.clear();
	}

	Value *existing = Get(key);

	if (existing) {
		*existing = value;
		existing->name = key;
		return *existing;
	}

	children.push_back(value);
	children.back().name = key;

	return children.back();
}

void json::Value::Erase(u32 index)
{
	if (index < children.size())
		children.erase(children.begin() + index);
}

void json::Value::SwapChildren(u32 a, u32 b)
{
	if (a < children.size() && b < children.size())
		std::swap(children[a], children[b]);
}

json::Value &json::Value::Push(const Value &value)
{
	if (type != Type_Array) {
		type = Type_Array;
		children.clear();
	}

	children.push_back(value);
	children.back().name.clear();

	return children.back();
}

String json::Quote(const String &str)
{
	String out;
	out.reserve(str.size() + 2);
	out.push_back('"');

	for (char c : str) {
		switch (c) {
			case '"':
				out += "\\\"";
				break;
			case '\\':
				out += "\\\\";
				break;
			case '\n':
				out += "\\n";
				break;
			case '\r':
				out += "\\r";
				break;
			case '\t':
				out += "\\t";
				break;
			case '\b':
				out += "\\b";
				break;
			case '\f':
				out += "\\f";
				break;
			default:
				if (static_cast<u8>(c) < 0x20) {
					char buf[8];
					snprintf(buf, sizeof(buf), "\\u%04x", static_cast<u32>(static_cast<u8>(c)));
					out += buf;
				} else {
					out.push_back(c);
				}
				break;
		}
	}

	out.push_back('"');
	return out;
}

void json::Value::WriteScalar(String *out) const
{
	char buf[64];

	switch (type) {
		case Type_Null:
			*out += "null";
			break;

		case Type_Bool:
			*out += bool_value ? "true" : "false";
			break;

		case Type_Int:
			snprintf(buf, sizeof(buf), "%lld", int_value);
			*out += buf;
			break;

		case Type_Double:
			snprintf(buf, sizeof(buf), "%.9g", double_value);
			*out += buf;
			break;

		case Type_String:
			*out += Quote(string_value);
			break;

		default:
			break;
	}
}

void json::Value::WriteCompact(String *out) const
{
	if (type == Type_Object) {
		out->push_back('{');

		for (u32 i = 0; i < children.size(); i++) {
			if (i > 0)
				out->push_back(',');

			*out += Quote(children[i].name);
			out->push_back(':');
			children[i].WriteCompact(out);
		}

		out->push_back('}');
	} else if (type == Type_Array) {
		out->push_back('[');

		for (u32 i = 0; i < children.size(); i++) {
			if (i > 0)
				out->push_back(',');

			children[i].WriteCompact(out);
		}

		out->push_back(']');
	} else {
		WriteScalar(out);
	}
}

String json::Value::Write() const
{
	String out;
	WriteCompact(&out);
	return out;
}

static void Indent(String *out, u32 count)
{
	for (u32 i = 0; i < count; i++)
		out->push_back('\t');
}

void json::Value::WritePrettyInto(String *out, u32 indent, u32 single_line_columns) const
{
	if (type != Type_Object && type != Type_Array) {
		WriteScalar(out);
		return;
	}

	const bool is_object = type == Type_Object;

	if (children.empty()) {
		*out += is_object ? "{}" : "[]";
		return;
	}

	bool flat = true;
	bool numeric = true;

	for (const Value &child : children) {
		if (child.IsObject() || child.IsArray())
			flat = false;

		if (!child.IsNumber())
			numeric = false;
	}

	// numeric arrays never wrap once they're on one line.
	bool wrap = is_object || !numeric;
	bool new_lines = !flat;

	u64 start = out->size();

	for (;;) {
		if (is_object)
			*out += new_lines ? "{\n" : "{ ";
		else
			*out += new_lines ? "[\n" : "[ ";

		bool restart = false;

		for (u32 i = 0; i < children.size(); i++) {
			const Value &child = children[i];

			if (new_lines)
				Indent(out, indent);

			if (is_object) {
				*out += Quote(child.name);
				*out += ": ";
			}

			child.WritePrettyInto(out, indent + 1, single_line_columns);

			if (i + 1 < children.size())
				out->push_back(',');

			out->push_back(new_lines ? '\n' : ' ');

			if (wrap && !new_lines && out->size() - start > single_line_columns) {
				out->resize(start);
				new_lines = true;
				restart = true;
				break;
			}
		}

		if (!restart)
			break;
	}

	if (new_lines)
		Indent(out, indent - 1);

	out->push_back(is_object ? '}' : ']');
}

String json::Value::WritePretty(u32 single_line_columns) const
{
	String out;
	WritePrettyInto(&out, 1, single_line_columns);
	return out;
}

struct Parser {
	const char *text;
	u64 length;
	u64 at;
	String error;

	bool Fail(const char *message)
	{
		if (error.empty()) {
			char buf[256];
			snprintf(buf, sizeof(buf), "%s (at offset %zu)", message, at);
			error = buf;
		}

		return false;
	}

	void SkipWhitespace()
	{
		while (at < length) {
			char c = text[at];

			if (c == ' ' || c == '\t' || c == '\n' || c == '\r') {
				at++;
			} else if (c == '/' && at + 1 < length && text[at + 1] == '/') {
				while (at < length && text[at] != '\n') {
					at++;
				}
			} else if (c == '/' && at + 1 < length && text[at + 1] == '*') {
				at += 2;

				while (at + 1 < length && !(text[at] == '*' && text[at + 1] == '/')) {
					at++;
				}

				at += 2;
			} else {
				break;
			}
		}
	}

	static void AppendUTF8(String *out, u32 cp)
	{
		if (cp < 0x80) {
			out->push_back(static_cast<char>(cp));
		} else if (cp < 0x800) {
			out->push_back(static_cast<char>(0xC0 | (cp >> 6)));
			out->push_back(static_cast<char>(0x80 | (cp & 0x3F)));
		} else if (cp < 0x10000) {
			out->push_back(static_cast<char>(0xE0 | (cp >> 12)));
			out->push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
			out->push_back(static_cast<char>(0x80 | (cp & 0x3F)));
		} else {
			out->push_back(static_cast<char>(0xF0 | (cp >> 18)));
			out->push_back(static_cast<char>(0x80 | ((cp >> 12) & 0x3F)));
			out->push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
			out->push_back(static_cast<char>(0x80 | (cp & 0x3F)));
		}
	}

	bool ParseString(String *out)
	{
		if (text[at] != '"')
			return Fail("expected string");

		at++;

		while (at < length) {
			char c = text[at++];

			if (c == '"')
				return true;

			if (c != '\\') {
				out->push_back(c);
				continue;
			}

			if (at >= length)
				break;

			char e = text[at++];

			switch (e) {
				case '"':
					out->push_back('"');
					break;
				case '\\':
					out->push_back('\\');
					break;
				case '/':
					out->push_back('/');
					break;
				case 'b':
					out->push_back('\b');
					break;
				case 'f':
					out->push_back('\f');
					break;
				case 'n':
					out->push_back('\n');
					break;
				case 'r':
					out->push_back('\r');
					break;
				case 't':
					out->push_back('\t');
					break;
				case 'u': {
					if (at + 4 > length)
						return Fail("bad unicode escape");

					char hex[5] = { text[at], text[at + 1], text[at + 2], text[at + 3], 0 };
					at += 4;

					AppendUTF8(out, static_cast<u32>(strtoul(hex, nullptr, 16)));
				} break;
				default:
					return Fail("bad escape");
			}
		}

		return Fail("unterminated string");
	}

	bool ParseNumber(json::Value *out)
	{
		u64 begin = at;
		bool is_double = false;

		if (text[at] == '-' || text[at] == '+')
			at++;

		while (at < length) {
			char c = text[at];

			if (c >= '0' && c <= '9') {
				at++;
			} else if (c == '.' || c == 'e' || c == 'E' || c == '+' || c == '-') {
				is_double = true;
				at++;
			} else {
				break;
			}
		}

		String token(text + begin, at - begin);

		if (token.empty() || token == "-" || token == "+")
			return Fail("bad number");

		if (is_double)
			*out = json::Value::MakeDouble(strtod(token.c_str(), nullptr));
		else
			*out = json::Value::MakeInt(strtoll(token.c_str(), nullptr, 10));

		return true;
	}

	bool Match(const char *word)
	{
		u64 n = strlen(word);

		if (at + n <= length && strncmp(text + at, word, n) == 0) {
			at += n;
			return true;
		}

		return false;
	}

	bool ParseValue(json::Value *out, u32 depth)
	{
		if (depth > 512)
			return Fail("nesting too deep");

		SkipWhitespace();

		if (at >= length)
			return Fail("unexpected end of input");

		char c = text[at];

		if (c == '{') {
			at++;
			*out = json::Value::MakeObject();

			for (;;) {
				SkipWhitespace();

				if (at < length && text[at] == '}') {
					at++;
					return true;
				}

				String key;

				if (!ParseString(&key))
					return false;

				SkipWhitespace();

				if (at >= length || text[at] != ':')
					return Fail("expected ':'");

				at++;

				json::Value child;

				if (!ParseValue(&child, depth + 1))
					return false;

				out->Set(key, child);

				SkipWhitespace();

				if (at < length && text[at] == ',') {
					at++;
					continue;
				}

				if (at < length && text[at] == '}') {
					at++;
					return true;
				}

				return Fail("expected ',' or '}'");
			}
		}

		if (c == '[') {
			at++;
			*out = json::Value::MakeArray();

			for (;;) {
				SkipWhitespace();

				if (at < length && text[at] == ']') {
					at++;
					return true;
				}

				json::Value child;

				if (!ParseValue(&child, depth + 1))
					return false;

				out->Push(child);

				SkipWhitespace();

				if (at < length && text[at] == ',') {
					at++;
					continue;
				}

				if (at < length && text[at] == ']') {
					at++;
					return true;
				}

				return Fail("expected ',' or ']'");
			}
		}

		if (c == '"') {
			String s;

			if (!ParseString(&s))
				return false;

			*out = json::Value::MakeString(s);
			return true;
		}

		if (Match("true")) {
			*out = json::Value::MakeBool(true);
			return true;
		}

		if (Match("false")) {
			*out = json::Value::MakeBool(false);
			return true;
		}

		if (Match("null")) {
			*out = json::Value::MakeNull();
			return true;
		}

		if (c == '-' || c == '+' || (c >= '0' && c <= '9'))
			return ParseNumber(out);

		return Fail("unexpected character");
	}
};

bool json::Parse(const String &text, Value *out, String *error)
{
	Parser parser;
	parser.text = text.c_str();
	parser.length = text.size();
	parser.at = 0;

	Value result;

	if (!parser.ParseValue(&result, 0)) {
		if (error)
			*error = parser.error;

		return false;
	}

	parser.SkipWhitespace();

	if (parser.at != parser.length) {
		if (error)
			*error = "trailing characters after document";

		return false;
	}

	*out = result;
	return true;
}
