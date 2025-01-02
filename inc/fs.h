// Copyright (c) 2021-2025, Implicit Conversions, Inc. Subject to the MIT License. See LICENSE file.
#pragma once

#include <vector>
#include <functional>
#include <string>

#include "StringUtil.h"

#if !defined(USE_STD_FILESYSTEM)
#	define USE_STD_FILESYSTEM    1
#endif

#if !defined(FILESYSTEM_NEEDS_OS_PATH)
#	if PLATFORM_LINUX || PLATFORM_OSX
#		define FILESYSTEM_NEEDS_OS_PATH		(0)
#	else
#		define FILESYSTEM_NEEDS_OS_PATH		(1)
#	endif
#endif

// Mount length describes the number of characters allowed prior to the colon, ex:
//    prefix:/path/to/file
//    ^^^^^^  length=6
// Windows (msw) mount length is generally considered one character. Some special platforms (android)
// may have longer mounts. Unix style systems ignore this setting.
#if !defined(FILESYSTEM_MOUNT_NAME_LENGTH)
#	define FILESYSTEM_MOUNT_NAME_LENGTH     (1)
#endif

// FILESYSTEM_MSW_MIXED_MODE - selects mixed mode paths, which use forward slash instead of backslash. Broadly,
// windows apps have graduated to a point of supporting mixed-mode paths in all but the most extreme legacy app
// situations (such as pathname parsing by `cmd.exe` and `start.exe`). In some cases mixed mode paths may parse
// more reliably than legacy native style paths using backslash. For this reason, forward slash is now preferred
// as the default path resolution mode.
#if !defined(FILESYSTEM_MSW_MIXED_MODE)
#	define FILESYSTEM_MSW_MIXED_MODE        (1)
#endif

namespace fs {

struct ReadSeekInterface {
	std::function<intmax_t(void* dest, intmax_t size)>    read;
	std::function<intmax_t(intmax_t pos, uint8_t whence)> seek;
	std::string FileObjectName;
};

class path;

bool		IsMswPathSep		(char c);
std::string ConvertFromMsw		(const std::string& msw_path, int maxMountLength=FILESYSTEM_MOUNT_NAME_LENGTH);
std::string ConvertToMsw		(const std::string& unix_path, int maxMountLength);
std::string ConvertToMswNative  (const std::string& unix_path, int maxMountLength);
std::string ConvertToMswMixed	(const std::string& unix_path, int maxMountLength);
std::string ConvertToMsw		(const std::string& unix_path);
std::string ConvertToMswNative	(const std::string& unix_path);
std::string ConvertToMswMixed	(const std::string& unix_path);
std::string PathFromString		(const char* path, int maxMountLength=FILESYSTEM_MOUNT_NAME_LENGTH);

bool		exists				(const path& path);
bool		remove				(const path& path);
bool		remove_all			(const path& path);
intmax_t	file_size			(const path& path);
bool		is_directory		(const path& path);
bool		create_directory	(const path& path);
bool		is_device			(const path& path);
std::string replace_extension	(const std::string& srcpath, const std::string& ext);
std::string remove_extension	(const std::string& srcpath, const std::string& ext_to_remove);
bool		stat				(const path& path, struct stat& st);		// use posix_stat instead. (this provided only for API compat with std::filesystem)
fs::path	absolute			(const path& fspath);

std::vector<path>	directory_iterator(const path& path);
void				directory_iterator(const std::function<void (const fs::path& path)>& func, const path& path);
std::vector<path>	recursive_directory_iterator(const path& path);

#if HAS_CHAR8_T
inline std::string PathFromString		(const char8_t* path) { return PathFromString((char*)path); }
#endif

extern std::string s_app0_dir;		// prepended to all relative paths when they are converted to libc-consumable strings.
extern void setAppRoot(std::string src);

class path
{
protected:
	std::string		uni_path_;			// universal path, path separators are forward slashes only (may include platform prefixes)
#if FILESYSTEM_NEEDS_OS_PATH
	std::string		libc_path_;			// native path expected by libc and such
#endif

	static const uint8_t separator = '/';

public:
	path() = default;
	path(const char* src) {
		uni_path_ = fs::PathFromString(src);
		update_native_path();
	}

	template<int len>
	path(const char (&src)[len]) {
		uni_path_ = fs::PathFromString(src);
		update_native_path();
	}

	path(char* (&src)) {
		uni_path_ = fs::PathFromString(src);
		update_native_path();
	}

#if HAS_CHAR8_T
	path(const char8_t* src) {
		uni_path_ = fs::PathFromString(src);
		update_native_path();
	}

	template<int len>
	path(const char8_t (&src)[len]) {
		uni_path_ = fs::PathFromString(src);
		update_native_path();
	}

	path(char8_t* (&src)) {
		uni_path_ = fs::PathFromString(src);
		update_native_path();
	}
#endif

	template<int len>
	fs::path& operator=(const char (&src)[len]) {
		uni_path_ = fs::PathFromString(src);
		update_native_path();
		return *this;
	}

	fs::path& operator=(char* (&src)) {
		uni_path_ = fs::PathFromString(src);
		update_native_path();
		return *this;
	}

	path(const std::string& src) {
		uni_path_ = fs::PathFromString(src.c_str());
		update_native_path();
	}

	// accepting implicit std::string conversions Causes too many problems with ambiguous assignments on clang.
	// Not worth the convenience of not typing .c_str() here or there.
	//fs::path& operator=(const std::string& src) {
	//	uni_path_ = fs::PathFromString(src.c_str());
	//	update_native_path();
	//	return *this;
	//}

	bool empty() const { return uni_path_.empty(); }
	void clear() {
		uni_path_.clear();
#if PLATFORM_SCE || PLATFORM_MSW
		libc_path_.clear();
#endif
	}

	path& append(const std::string& comp);
	path& concat(const std::string& src);

	// immutable extention replacement, intentionally differs from STL's mutable version, because
	// no one wants or expects mutable string/path operations in this context.
	path replace_extension(const std::string& extension) const {
		return fs::replace_extension(uni_path_, extension).c_str();
	}

	// Full extension of path, including first '.'
	std::string extension() const {
		// implementation note: returns a string because a filename will itself never have
		// variances based on host OS / platform.

		auto fn = filename();
		if (fn.find_first_not_of('.') != fn.npos) {
			auto pos = uni_path_.find_last_of('.');
			if (pos != std::string::npos)
				return uni_path_.substr(pos);
		}

		return {};
	}

	std::string filename() const {
		// implementation note: returns a string because a filename will itself never have
		// variances based on host OS / platform.
		auto pos = uni_path_.find_last_of(separator);
		if (pos != std::string::npos)
			return uni_path_.substr(pos + 1);

		return uni_path_;
	}

	path parent_path() const {
		auto pos = uni_path_.find_last_of(separator);
		if (pos != std::string::npos)
			return uni_path_.substr(0, pos).c_str();

		return *this;
	}

	/**
	 * Returns a list of the path components.
	 */
	std::vector<std::string> split_path() const {
		if (uni_path_.empty()) {
			return {};
		}

		std::vector<std::string> result;
		size_t index = 0;

		// Absolute path
		if (uni_path_[0] == '/') {
			result.push_back("/");
			index++;
		}

		char delims[2] = { (char)separator, 0 };

		for (;;) {
			if (auto pos = uni_path_.find(separator, index); pos != std::string::npos) {
				std::string token = StringUtil::trim(uni_path_.substr(index, pos-index), delims);
				if (!token.empty()) {
					result.emplace_back(std::move(token));
				}
				index = pos + 1;
			}
			else {
				auto token = StringUtil::trim(uni_path_.substr(index));
				if (!token.empty()) {
					result.emplace_back(std::move(token));
				}
				break;
			}
		}

		return result;
	}


	bool is_absolute() const {
		return (!uni_path_.empty() && uni_path_[0] == separator);
	}

	bool is_device() const {
		if (uni_path_.empty() || uni_path_[0] != '/') return 0;		// shortcut early out.

		if (uni_path_ == "/dev/null") return 1;
		if (uni_path_ == "/dev/tty" ) return 1;
		if (StringUtil::BeginsWith(uni_path_, "/dev/null/")) return 1;
		if (StringUtil::BeginsWith(uni_path_, "/dev/tty/" )) return 1;

		return 0;
	}

	// typical use case is starts_with argument should always begin with a forward slash "/"
	bool starts_with(std::string const& rval) {
		return StringUtil::BeginsWith(uni_path_, rval);
	}

	// POSIX style alias for C++ 'parent_path()'
	[[nodiscard]] path dirname() const { return parent_path(); }

	// std::string conversions
	[[nodiscard]] const char* c_str				   () const { return libc_path().c_str(); }
	[[nodiscard]] const std::string& string		   () const { return libc_path(); }
	[[nodiscard]] const std::string& u8string	   () const { return libc_path(); }
	[[nodiscard]] const std::string& native        () const { return libc_path(); }
	[[nodiscard]] const std::string& uni_string	   () const { return uni_path_; }		// uni - unified (or unix-style)
	[[nodiscard]] const std::string& generic_string() const { return uni_path_; }		// generic is same as uni, provided for std::filesystem compat

	[[nodiscard]] operator const char*			   () const { return libc_path().c_str(); }
	[[nodiscard]] operator const std::string&	   () const { return libc_path(); }

	bool  operator == (const path& s)           const;
	bool  operator == (const char *s)           const;

	// three-way comparison rules can cause errors when using comparisons on certain STL constainers (std::vector).
	// Avoiding definition of != when the compiler supports three-way comparison fixes the problem.
	//   (vfixes MSC error: "rewritten candidate function was excluded from overload resolution because a corresponding operator!= declared in the same scope")
#if !defined(__cpp_impl_three_way_comparison) || (__cpp_impl_three_way_comparison < 201907)
	bool  operator != (const path& s)           const;
	bool  operator != (const char *s)           const;
#endif

	bool  operator >  (const path& s)           const;
	bool  operator >= (const path& s)           const;
	bool  operator <  (const path& s)           const;
	bool  operator <= (const path& s)           const;

	path& operator /= (const fs::path& fpath)         { return append(fpath.uni_path_); }
	path  operator /  (const fs::path& fpath)   const { return path(*this).append(fpath.uni_path_); }

	template<int len>
	path& operator /= (const char (&src)[len])        { return append(src); }
	template<int len>
	path  operator /  (const char (&src)[len])  const { return path(*this).append(src); }

	path& operator /= (const char *src)               { return append(src); }
	path  operator /  (const char *src)         const { return path(*this).append(src); }

	path& operator /= (const std::string& name)       { return append(name); }
	path  operator /  (const std::string& name) const { return path(*this).append(name); }
	path  operator +  (const std::string& ext)  const { return path(*this).concat(ext); }
	path& operator += (const std::string& ext)	      { return concat(ext); }

	[[nodiscard]] [[deprecated("use native()")]] std::string asLibcStr() const { return libc_path(); }

	[[nodiscard]] static std::string asLibcStr(const char* src) {
		return fs::path(src).native();
	}
	[[nodiscard]] static std::string asLibcStr(const fs::path& src) {
		return src.native();
	}
public:
	[[nodiscard]] std::string& raw_modifiable_uni () { return uni_path_; }
	void raw_commit_modified() { update_native_path(); }

protected:
	[[nodiscard]] const std::string& libc_path() const;
	void update_native_path();
};

} // namespace fs
