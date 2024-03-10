#pragma once

#include <sstream>
#include <string>
#include <vector>
#include <optional>
#include <algorithm>
#include <limits>
#include <tuple>

#include <cstdarg>
#include <cstring>

#include "jfmt.h"
#include "StdStringArg.h"
#include "StringBuilder.h"

#if !defined(__verify_fmt)
#   if defined(_MSC_VER)
#   	define __verify_fmt(fmtpos, vapos)
#   else
#   	define __verify_fmt(fmtpos, vapos)  __attribute__ ((format (printf, fmtpos, vapos)))
#   endif
#endif

#if defined(__MINGW64__)
	// MINGW64 defines strcasecmp and strncasecmp itself as macros in <cstring> but this interferes
	// with our desire to use function overloading in C++.
#	undef strcasecmp
#	undef strncasecmp
#endif

#if defined(_MSC_VER) || defined(__MINGW64__)
#	define HAS_strcasestr   1
	extern char *_stristr(const char *haystack, const char *needle);
	inline auto strcasestr  (char const* a, char const* b)              { return _stristr (a,b); }
#endif

#if defined(_MSC_VER)
	inline auto strcasecmp  (char const* a, char const* b)              { return _stricmp (a,b); }
	inline auto strncasecmp (char const* a, char const* b, ptrdiff_t c) { return _strnicmp(a,b,c); }
#endif

#if !defined(HAS_strcasestr)
#	if defined(_GNU_SOURCE)
#	   define HAS_strcasestr   1
#	else
#	   define HAS_strcasestr   0
#	endif
#endif

#if !HAS_strcasestr
/*
 * Find the first occurrence of find in s, ignore case.
 */
inline const char *strcasestr(const char *s, const char *find) {
	char c, sc;
	size_t len;

	if ((c = *find++) != 0) {
		c = (char)tolower((unsigned char)c);
		len = strlen(find);
		do {
			do {
				if ((sc = *s++) == 0)
					return (NULL);
			} while ((char)tolower((unsigned char)sc) != c);
		} while (strncasecmp(s, find, len) != 0);
		s--;
	}
	return ((char *)s);
}
#endif

// These uint8_t variants are convenient for old C++, but will probably fail hard in a world with char8_t.
// (hopefully at that point we can just ifdef them away or whatever. --jstine)

inline auto strcasestr  (uint8_t const* a, uint8_t const* b)              { return strcasestr  ((char const*)a, (char const*)b); }
inline auto strcasecmp  (uint8_t const* a, uint8_t const* b)              { return strcasecmp  ((char const*)a, (char const*)b); }
inline auto strncasecmp (uint8_t const* a, uint8_t const* b, ptrdiff_t c) { return strncasecmp ((char const*)a, (char const*)b,c); }

// snprintf is usually preferred over other variants:
//   - snprintf_s has annoying parameter validation and nullifies the buffer instead of truncate.
//   - sprintf_s  has annoying parameter validation and nullifies the buffer instead of truncate.
//   - snprintf   truncates the result and ensures a null terminator.
template<intmax_t _Size> yesinline __verify_fmt(2, 3)
int snprintf(char (&_Buf)[_Size], const char *_Fmt, ...)
{
	int _Res;
	va_list _Ap;

	va_start(_Ap, _Fmt);
	_Res = vsnprintf(_Buf, _Size, _Fmt, _Ap);
	va_end(_Ap);

	return _Res;
}

template<intmax_t _Size> yesinline
char const* fgets(char (&_Buf)[_Size], FILE* fp) {
	return fgets(_Buf, _Size, fp);
}

// Neat!  Returns the case option as a string matching precisely the case label. Useful for logging
// hardware registers and for converting sparse enumerations into strings (enums where simple char*
// arrays fail).
#define CaseReturnString(caseName)        case caseName: return # caseName


// filename illegals, for use with ReplaceCharSet, to replace with underscore (_)
static const char msw_fname_illegalChars[] = "\\/:?\"<>|";

namespace StringUtil {

	// deprecated, prefer 'StartsWith', which aligns with C++20 standard conventions.
	yesinline inline bool BeginsWith(const std::string& left, char right) {
		return !left.empty() && (left[0] == right);
	}

	// deprecated, prefer 'StartsWith', which aligns with C++20 standard conventions.
	yesinline inline bool BeginsWith(const std::string& left, const std::string& right) {
		return left.compare( 0, right.length(), right) == 0;
	}

	yesinline inline bool StartsWith(const std::string& left, char right) {
		return !left.empty() && (left[0] == right);
	}

	yesinline inline bool StartsWith(const std::string& left, const std::string& right) {
		return left.compare( 0, right.length(), right) == 0;
	}

	yesinline inline bool EndsWith(const std::string& left, const std::string& right) {
		intmax_t startpos = left.length() - right.length();
		if (startpos<0) return false;
		return left.compare( startpos, right.length(), right ) == 0;
	}

	yesinline inline bool EndsWith(const std::string& left, char right) {
		return !left.empty() && (left[left.length()-1] == right);
	}

	inline std::vector<std::string> Split(const std::string& str, char delimiter) {
		std::vector<std::string> parts;
		std::istringstream iss(str);

		std::string token;
		while (std::getline(iss, token, delimiter)) {
			parts.push_back(token);
		}
		return parts;
	}

	template<class StdStrT> void AppendFmtV(StdStrT& result, const StringConversionMagick& fmt, va_list list);
	template<class StdStrT> void AppendFmt (StdStrT& result, const char* fmt, ...) __verify_fmt(2,3);

	extern std::string		FormatV		(const StringConversionMagick& fmt, va_list list);
	extern std::string		Format		(const char* fmt, ...)							__verify_fmt(1,2);
	extern std::string  	trim		(const std::string& s, const char* delims = " \t\r\n");
	extern std::string  	toLower		(std::string s);
	extern std::string  	toUpper		(std::string s);
	extern std::string  	ReplaceCharSet(std::string srccopy, const char* to_replace, char new_ch);
	extern bool				globMatch	(char const* pattern, char const* candidate);

	extern bool getBoolean(const StringConversionMagick& left, bool* parse_error=nullptr);

	extern ptrdiff_t CompareCase(std::string_view lval, std::string_view rval);		// equiv to strcasecmp
	extern ptrdiff_t FindFirstCase(std::string_view s, std::string_view find);		// equiv to strcasestr

	// returns { result, error } -- result will be defbool if error occurred (error=true).
	yesinline inline std::tuple<bool, bool> getBoolean(const StringConversionMagick& left, bool defbool) {
		bool error;
		auto result = getBoolean(left, &error);
		return { error ? defbool : result, error };
	}

	yesinline inline std::string	ReplaceString(std::string subject, std::string_view search, std::string_view replace) {
		size_t pos = 0;
		while ((pos = subject.find(search, pos)) != std::string::npos) {
			subject.replace(pos, search.size(), replace);
			pos += replace.size();
		}
		return subject;
	}

	yesinline inline std::string	ReplaceCase(std::string subject, std::string_view search, std::string_view replace) {
		ptrdiff_t pos;
		while ((pos = FindFirstCase(subject, search)) != -1) {
			subject.replace(pos, search.size(), replace);
			pos += search.size();
		}
		return subject;
	}

	std::string LineNumberString(const char* str);
	inline std::string LineNumberString(const std::string_view& str) {
		return LineNumberString(str.data());
	}
}

extern int strcpy_ajek(char* dest, int destlen, const char* src);
template<int size> yesinline
int strcpy_ajek(char (&dest)[size], const char* src)
{
	return strcpy_ajek(dest, size, src);
}

// returns TRUE if the argument is nullptr or empty string.
yesinline inline bool strIsEmpty(StringConversionMagick const& view) {
	return view.empty();
}

// This verification extension is provided via force-include to help ensure it is exposed to all TUs. If the
// project does not include the forceinlue then we placebo it here (treating it as an optional static check
// extension).

#if !defined(VERIFY_PRINTF_ON_MSVC)
#	define VERIFY_PRINTF_ON_MSVC(...)	(void(0))
#endif

// Macros
//  cFmtStr - Format String with c_str (ASCII-Z) return type  <-- useful for printf, most C APIs
//  sFmtStr - Format String with STL return type	<-- mostly to provide matching API for cFmtStr macro

#if defined(VERIFY_PRINTF_ON_MSVC)
#	define sFmtStr(...)		            (VERIFY_PRINTF_ON_MSVC(__VA_ARGS__), StringUtil::Format(__VA_ARGS__)        )
#	define cFmtStr(...)		            (StringBuilder<256>().format(__VA_ARGS__).c_str())
#	define sAppendFmt(dest, fmt, ...)	(VERIFY_PRINTF_ON_MSVC(fmt, __VA_ARGS__), StringUtil::AppendFmt(dest, fmt, ## __VA_ARGS__))
#else
#	define sFmtStr(...)		            (StringUtil::Format(__VA_ARGS__)        )
#	define cFmtStr(...)		            (StringBuilder<256>().format(__VA_ARGS__).c_str())
#	define sAppendFmt(dest, fmt, ...)	(StringUtil::AppendFmt(dest, fmt, ## __VA_ARGS__))
#endif

extern double CvtTimePostfixToScalar(char const* endptr);
extern double CvtNumericalPostfixToScalar(char const* endptr);

extern bool u8_nul_or_whitespace(uint8_t ch);

constexpr uint32_t hash(std::string_view data) noexcept {
	uint32_t hash = 5381;

	for (auto c : data) {
		hash = ((hash << 5) + hash) + c;
	}

	return hash;
}

// shorthand to quickly parse size-type arguments, which are expected to be positive integers.
// malformed arguments and negative integers return FALSE and leave the dest unmodified.
template< typename T >
bool StrParseSizeArg(char const* src, T& dest) {
	if (!src || !src[0]) {
		return false;
	}

	char* endptr = nullptr;
	if (auto newval = strtosj(src, &endptr, 0); endptr && endptr != src) {
		newval *= CvtNumericalPostfixToScalar(endptr);
		if (newval > 0) {
			dest = (T)newval;
			return true;
		}
	}

	return false;
}

std::string SanitizeUtf8(const std::string& str);


namespace StringUtil::_template_impl {
	template<bool isSigned>
	auto _strtoj_tmpl(char const* src, char** endptr=nullptr, int radix=0) {
		if constexpr(isSigned) {
			return strtosj(src, endptr, radix);
		}
		else {
			return strtouj(src, endptr, radix);
		}
	}

	extern std::optional<bool> ConvertToBool(StringConversionMagick const& rval);

	template<typename T>
	std::optional<T> ConvertFromString_integrals(StringConversionMagick const& rval) {
		if (rval.empty()) {
			return {};
		}

		auto result = _strtoj_tmpl<std::is_signed_v<T>> (rval.c_str());
		if (result != (T)result) {
			errno = ERANGE;
			return { std::clamp<decltype(result)>(result, std::numeric_limits<T>::min(), std::numeric_limits<T>::max()) };
		}
		return { result };
	}

	std::optional<double> ConvertFromString_f64(StringConversionMagick const& rval);
	std::optional<float > ConvertFromString_f32(StringConversionMagick const& rval);
}

namespace StringUtil {
	template<typename T>
	std::optional<T> Parse(StringConversionMagick const& rval);
}
