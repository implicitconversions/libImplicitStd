
#include <string>
#include <set>

#include "icy_log.h"
#include "icy_assert.h"
#include "fs.h"

#if PLATFORM_MSW
#	include <sys/types.h>
#	include <sys/stat.h>
#endif

#if !defined(elif)
#	define elif		else if
#endif

namespace fs
{
std::string s_app0_dir;

// Applies an AppRoot for the entire process.
// This workaround is provided for packaged environments that don't provide an option to modify the CWD
// at runtime - like Game Consoles and Mobile Apps.
void setAppRoot(std::string src) {
	s_app0_dir = src;

	if (!StringUtil::EndsWith(s_app0_dir, ('/' )) &&
		!StringUtil::EndsWith(s_app0_dir, ('\\'))
	) {
		s_app0_dir.push_back((PLATFORM_MSW && !FILESYSTEM_MSW_MIXED_MODE) ? '\\' : '/');
	}
}

std::string remove_extension(const std::string& path, const std::string& ext_to_remove) {

	if (ext_to_remove.empty()) {
		return replace_extension(path, "");
	}

	if (StringUtil::EndsWith(path, ext_to_remove)) {
		return path.substr(0, path.length() - ext_to_remove.length());
	}

	return path;
}

std::string replace_extension(const std::string& path, const std::string& extension) {
	// impl note: extension replacement is a platform-agnostic action.
	// there's no need to have fs::path involved in this process.

	auto pos = path.find_last_of('.');

	// C++ STL conformance:
	//  * append new extension, even if none previously existed.
	//  * append a dot automatically if none specified.
	//  * remove previous extension and then do nothing if specified extension is empty (do not append dot)

	std::string result = path;
	if (pos != std::string::npos) {
		result.erase(pos);
	}
	if (!extension.empty()) {
		if (extension[0] != '.') {
			result += '.';
		}
		result += extension;
	}
	return result;
}

bool IsMswPathSep(char c)
{
	return (c == '\\') || (c == '/');
}

// intended for use on fullpaths which have already had host prefixes removed.
std::string ConvertFromMsw(const std::string& origPath, int maxMountLength)
{
	std::string result;

	// peculiar windows: it has some hard-coded filenames which are not prefixed or post-fixed by
	// anything. Which would make them really dangerous and invasive to filesystem behavior, but hey
	// that's how windows rolls.
	//
	// full list of reserved (most of which we don't check for or handle)
	//
	// PRN.
	// AUX
	// NUL
	// COM1, COM2, COM3, COM4, COM5, COM6, COM7, COM8, COM9, COM0
	// LPT1, LPT2, LPT3, LPT4, LPT5, LPT6, LPT7, LPT8, LPT9, LPT0

	if (origPath == "NUL") {
		return "/dev/null";
	}

	if (origPath == "CON") {
		return "/dev/tty";
	}

	// max output length is original length plus drive specifier, eg. /c  (two letters)
	result.resize(origPath.length() + 3);
	const char* src = origPath.c_str();
		  char* dst = &result[0];

	// Typically a conversion from windows to unix style path has a 1:1 length match.
	// The problem occurs when the path isn't rooted, eg.  c:some\dir  vs. c:\some\dir
	// In the former case, windows keeps a CWD for _every_ drive, and there's no sane way
	// to safely encode that as a unix-style path.  The painful option is to get the CWD
	// for the drive letter and paste it in.  That sounds like work!
	//
	// The real problem is that it's a super-dodgy "feature" of windows, with no easy way
	// to get the CWD of the drive without doing some very racy operations with setcwd/getcwd.
	// More importantly, it literally isn't replicated any other operating system, which means
	// it is very difficult to port logic that somehow relies on this feature.  In the interest
	// of cross-platform support, we detect this and throw a hard error rather than try to support it.

	if (maxMountLength == 1) {
		if (isalnum((uint8_t)src[0]) && src[1] == ':') {
			dst[0] = '/';
			dst[1] = tolower(src[0]);

			// early-exit to to allow `c:` -> `/c`
			// this conversion might be useful for internal path parsing and is an unlikely source of user error.

			if (!src[2]) {
				return result;
			}

			//house_check (IsMswPathSep(src[2]),
			//	"Invalid msw-specific non-rooted path with drive letter: %s\n\n"
			//	"Non-rooted paths of this type are not supported to non-standard\n"
			//	"and non-portable nature of the specification.\n",
			//	origPath.c_str()
			//);

			src  += 2;
			dst  += 2;
		}
	}
	elif (strchr(src, ':')) {
		// found a colon. Rewrite the path.
		*dst++ = '/';
		while (src[0] && src[0] != ':') {
			*dst++ = *src++;
		}

		// early-exit to to allow `rom:` -> `/rom`
		// this conversion might be useful for internal path parsing and is an unlikely source of user error.
		if (!src) {
			return result;
		}
		++src; // advance past colon.
		src += (src[0] == '/' || src[0] == '\\');
		*dst++ = '/';
	}

	// - a path that starts with a single backslash is always rejected.
	// - a path that starts with a single forward slash is only rejected if it doesn't _look_ like a
	//   drive letter spec.
	//       /c/woombey/to  <-- OK!
	//       /woombey/to    <-- OK only if (maxMountLength > 1)

	elif (src[0] == '\\') {
		if (src[0] == src[1]) {
			// network name URI, don't do anything (regular slash conversion is OK)
		}
		else {
			fprintf( stderr, "Invalid path layout: %s\n"
				"Rooted paths without drive specification are not allowed.\n"
				"Please explicitly specify the drive letter in the path.\n",
				origPath.c_str()
			);
			return {};
		}
	}
	elif (src[0] == '/') {
		if (src[0] == src[1]) {
			// network name URI, don't do anything (regular slash conversion is OK)
		}
	}

	// copy rest of the string char for char, replacing '\\' with '/'
	for(; src[0]; ++src, ++dst) {
		dst[0] = (src[0] == '\\') ? '/' : src[0];
	}
	dst[0] = 0;
	assertD((dst - &result[0]) < result.size(), "Original=%s result=%s", origPath.c_str(), result.c_str());
	result.resize(dst - &result[0]);
	return result;
}

// intended for use on fullpaths which have already had host prefixes removed.
template<bool MixedMode>
std::string _tmpl_ConvertToMsw(const std::string& unix_path, int maxMountLength)
{
	if (unix_path.empty()) {
		return {};
	}

	const char* src = unix_path.c_str();

	// null and tty automatically disregard subdirectories, as a convenience to programming paradigms.
	// If a component sets a root dir to /dev/null then all files supposed to be created under that dir
	// will become pipes in/out of /dev/null

	bool append_approot = 1;

	if (src[0] == '/') {
		if (unix_path == "/dev/null" || StringUtil::BeginsWith(unix_path, "/dev/null/")) {
			return "NUL";
		}

		if (unix_path == "/dev/tty" || StringUtil::BeginsWith(unix_path, "/dev/tty/")) {
			return "CON";
		}
	}

	std::string result;
	result.resize(unix_path.length());
	char* dst = &result[0];
	if (src[0] == '/' && isalnum((uint8_t)src[1]) && src[2] == '/') {
		dst[0] = toupper(src[1]);
		dst[1] = ':';
		src  += 2;
		dst  += 2;
		append_approot = 0;
	}
	else if (src[0] == '.' && (src[1] == '/')) {
		// relative to current dir, just strip the ".\"
		src += 2;
	}
	else if (src[0] == '/') {
		append_approot = 0;

		// treat /cwd/ in a special way - it gets stripped and is -not- replaced with s_app0_dir
		if (StringUtil::BeginsWith(unix_path, "/cwd/")) {
			src += 5;
		}
		elif (auto slash = unix_path.find_first_of('/', 1); slash < maxMountLength || unix_path.size() <= maxMountLength) {
			// found a slash within the length limit.
			// convert into a mount name prefix.
			++src;
			while (src[0] && src[0] != '/') {
				*dst++ = *src++;
			}
			*dst++ = ':';
			if (src[0] == '/') {
				*dst++ = MixedMode ? '/' : '\\';
				++src;
			}
		}
		else {
			// special roots don't get backslash conversion. They're not valid paths anyway
			// so assume the user knows what they're doing for some special purpose/reason.
			is_special_root = 1;
		}
	}

	if (is_special_root) {
		for(; src[0]; ++src, ++dst) {
			dst[0] = src[0];
		}
		dst[0] = 0;
	}
	else {
		if constexpr (MixedMode) {
			// copy rest of the string char for char (mixed mode keeps forward slashes)
			for(; src[0]; ++src, ++dst) {
				dst[0] = src[0];
			}
		}
		else {
			// copy rest of the string char for char, replacing '/' with '\\'
			for(; src[0]; ++src, ++dst) {
				dst[0] = (src[0] == '/') ? '\\' : src[0];
			}
		}
		dst[0] = 0;
	}

	ptrdiff_t newsize = dst - result.c_str();
	assertH(newsize <= ptrdiff_t(unix_path.length()));

	if (append_approot) {
		return s_app0_dir + result;
	}
	else {
		result.resize(newsize);
		return result;
	}
}

path& path::append(const std::string& comp)
{
	if (comp.empty()) return *this;

	const auto& unicomp = PathFromString(comp.c_str());
	if (unicomp[0] == '/') {
		// provided path is rooted, thus it overrides original path completely.
		uni_path_ = unicomp;
	}
	else {
		if (!uni_path_.empty() && !StringUtil::EndsWith(uni_path_, '/')) {
			uni_path_ += '/';
		}
		uni_path_ += unicomp;
	}
	update_native_path();
	return *this;
}

path& path::concat(const std::string& src)
{
	// implementation mimics std::filesystem in that it performs no platform-specific
	// interpretation and expects the caller to "know what they're doing."  If the incoming
	// string contains backslashes they will be treated as literal backslashes (valid filename
	// characters on a unix filesystem).

	uni_path_  += src;
#if FILESYSTEM_NEEDS_OS_PATH
	libc_path_ += src;
#endif
	return *this;
}

fs::path absolute(const path& fspath) {
	return fs::path(s_app0_dir) / fspath;
}

static constexpr bool MIXED_MODE = true;
static constexpr bool NATIVE_MODE = false;

std::string ConvertToMsw(const std::string& unix_path) {
	constexpr auto mode = FILESYSTEM_MSW_MIXED_MODE ? MIXED_MODE : NATIVE_MODE;
	return _tmpl_ConvertToMsw<mode>(unix_path, FILESYSTEM_MOUNT_NAME_LENGTH);
}

std::string ConvertToMsw(const std::string& unix_path, int maxMountLength) {
	constexpr auto mode = FILESYSTEM_MSW_MIXED_MODE ? MIXED_MODE : NATIVE_MODE;
	return _tmpl_ConvertToMsw<mode>(unix_path, maxMountLength);
}

std::string ConvertToMswNative(const std::string& unix_path) {
	return _tmpl_ConvertToMsw<NATIVE_MODE>(unix_path, FILESYSTEM_MOUNT_NAME_LENGTH);
}

std::string ConvertToMswNative(const std::string& unix_path, int maxMountLength) {
	return _tmpl_ConvertToMsw<NATIVE_MODE>(unix_path, maxMountLength);
}

std::string ConvertToMswMixed(const std::string& unix_path) {
	return _tmpl_ConvertToMsw<MIXED_MODE>(unix_path, FILESYSTEM_MOUNT_NAME_LENGTH);
}

std::string ConvertToMswMixed(const std::string& unix_path, int maxMountLength) {
	return _tmpl_ConvertToMsw<MIXED_MODE>(unix_path, maxMountLength);
}

}
