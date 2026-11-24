#include "io/path.h"

#include "core/types.h"
#include "core/str.h"
#include "container/vector.h"

static u64 LastSlash(const String &path)
{
	u64 slash = path.find_last_of("/\\");
	return slash;
}

String path::GetFileName(const String &path)
{
	String name = GetFileNameExt(path);
	u64 dot = name.find_last_of('.');

	if (dot == String::npos)
		return name;

	return name.substr(0, dot);
}

String path::GetFileNameExt(const String &path)
{
	u64 slash = LastSlash(path);

	if (slash == String::npos)
		return path;

	return path.substr(slash + 1);
}

String path::GetFileExtension(const String &path)
{
	u64 dot = path.find_last_of('.');
	u64 slash = LastSlash(path);

	if (dot == String::npos || (slash != String::npos && dot < slash))
		return String();

	return path.substr(dot);
}

String path::GetFilePathNoExt(const String &path)
{
	u64 dot = path.find_last_of('.');
	u64 slash = LastSlash(path);

	if (dot == String::npos || (slash != String::npos && dot < slash))
		return path;

	return path.substr(0, dot);
}

String path::GetFileDirectory(const String &path)
{
	u64 slash = path.find_last_of('/');

	if (slash == String::npos)
		return String();

	return path.substr(0, slash + 1);
}

String path::Join(const String &path_a, const String &path_b)
{
	if (path_a.empty())
		return Normalize(path_b);

	if (path_b.empty())
		return Normalize(path_a);

	return path_a + "/" + path_b;
}

String path::SanitizeFileName(const String &name, const String &fallback)
{
	String lower = str::ToLower(str::Trim(name));
	String out;
	bool in_bad_run = false;

	for (char c : lower) {
		bool ok = (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_' || c == '-';

		if (ok) {
			out += c;
			in_bad_run = false;
		} else if (!in_bad_run) {
			out += '_';
			in_bad_run = true;
		}
	}

	if (out.empty())
		return fallback;

	return out;
}

String path::Normalize(const String &path)
{
	Vector<String> parts;
	String current;

	bool absolute = !path.empty() && (path[0] == '/' || path[0] == '\\');

	for (u64 i = 0; i <= path.size(); i++) {
		char c = (i < path.size()) ? path[i] : '/';

		if (c == '/' || c == '\\') {
			if (current.empty() || current == ".") {
			} else if (current == "..") {
				if (!parts.empty() && parts.back() != "..")
					parts.pop_back();
				else if (!absolute)
					parts.push_back(current);
			} else {
				parts.push_back(current);
			}

			current.clear();
		} else {
			current += c;
		}
	}

	String result = absolute ? "/" : "";

	for (u64 i = 0; i < parts.size(); i++) {
		result += parts[i];
		result += "/";
	}

	bool trailing_slash = !path.empty() && (path.back() == '/' || path.back() == '\\');

	if (!trailing_slash && !result.empty() && result.size() > 1)
		result.pop_back();

	return result;
}
