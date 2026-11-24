#include "core/str.h"
#include "core/types.h"

#include <cctype>

String str::ToLower(const String &value)
{
	String out = value;
	for (char &c : out) c = std::tolower(c);
	return out;
}

String str::Trim(const String &value)
{
	u64 start = 0;
	u64 end = value.size();

	while (start < end && std::isspace(value[start])) {
		start++;
	}

	while (end > start && std::isspace(value[end - 1])) {
		end--;
	}

	return value.substr(start, end - start);
}

bool str::StartsWith(const String &value, const String &prefix)
{
	if (prefix.size() > value.size())
		return false;

	return value.compare(0, prefix.size(), prefix) == 0;
}

bool str::EndsWith(const String &value, const String &suffix)
{
	if (suffix.size() > value.size())
		return false;

	return value.compare(value.size() - suffix.size(), suffix.size(), suffix) == 0;
}
