#pragma once

#include <string>
#include <vector>

#include <cstdarg>

#if !defined(__verify_fmt)
#   if defined(_MSC_VER)
#   	define __verify_fmt(fmtpos, vapos)
#   else
#   	define __verify_fmt(fmtpos, vapos)  __attribute__ ((format (printf, fmtpos, vapos)))
#   endif
#endif

#if defined(_MSC_VER)
#	define strcasecmp(a,b)		_stricmp(a,b)
#	define strcasestr(a,b)		_stristr(a,b)
#	define strncasecmp(a,b,c)	_strnicmp(a,b,c)

extern char *_stristr(const char *haystack, const char *needle);
#endif

#if !defined(HAS_strcasestr)
#   define HAS_strcasestr   1
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

// snprintf is usually preferred over other variants:
//   - snprintf_s has annoying parameter validation and nullifies the buffer instead of truncate.
//   - sprintf_s  has annoying parameter validation and nullifies the buffer instead of truncate.
//   - snprintf   truncates the result and ensures a null terminator.
template<rsize_t _Size> __verify_fmt(2, 3)
int snprintf(char (&_Buf)[_Size], const char *_Fmt, ...)
{
	int _Res;
	va_list _Ap;

	va_start(_Ap, _Fmt);
	_Res = vsnprintf(_Buf, _Size, _Fmt, _Ap);
	va_end(_Ap);

	return _Res;
}

// Neat!  Returns the case option as a string matching precisely the case label. Useful for logging
// hardware registers and for converting sparse enumerations into strings (enums where simple char*
// arrays fail).
#define CaseReturnString(caseName)        case caseName: return # caseName


// filename illegals, for use with ReplaceCharSet, to replace with underscore (_)
static const char msw_fname_illegalChars[] = "\\/:?\"<>|";

///////////////////////////////////////////////////////////////////////////////////////////////////
// StringConversionMagick - struct meant for use as an aide in function parameter passing only.
//
// Rationale:
//   C++ has a rule that we can't use references as the fmt parameter to a varadic function,
//   which prevents us from making a nice API that can accept std::string or const char* implicitly.
//   In the past I've worked around this by making my own wrapper class for std::string that has
//   an implicit conversion operator for const char*(), but that's a pretty heavyweight approach that
//   makes it really hard for libraries to inter-operate with other libs that expect plain old
//   std::string.  So now I'm going with this, let's see what happens!  --jstine
//
struct StringConversionMagick
{
	const char*			m_cstr   = nullptr;
	const std::string*	m_stdstr = nullptr;
	int                 m_length = -1;

	StringConversionMagick(const std::string& str) {
		m_stdstr = &str;
	}

	StringConversionMagick(const char* const (&str)) {
		m_cstr   = str;
	}

	template<int size>
	StringConversionMagick(const char (&str)[size]) {
		m_cstr   = str;
		m_length = size-1;
	}

	const char* c_str() const {
		return m_stdstr ? m_stdstr->c_str() : m_cstr;
	}

	bool empty() const {
		if (m_stdstr) {
			return m_stdstr->empty();
		}
		return !m_cstr || !m_cstr[0];
	}

	auto length() const {
		if (m_stdstr) return m_stdstr->length();
		return (m_length < 0) ? strlen(m_cstr) : m_length;
	}
};

namespace StringUtil {

	__nodebug inline bool BeginsWith(const std::string& left, char right) {
		return !left.empty() && (left[0] == right);
	}

	__nodebug inline bool BeginsWith(const std::string& left, const std::string& right) {
		return left.compare( 0, right.length(), right) == 0;
	}

	__nodebug inline bool EndsWith(const std::string& left, const std::string& right) {
		intmax_t startpos = left.length() - right.length();
		if (startpos<0) return false;
		return left.compare( startpos, right.length(), right ) == 0;
	}

	__nodebug inline bool EndsWith(const std::string& left, char right) {
		return !left.empty() && (left[left.length()-1] == right);
	}

	extern void				AppendFmtV	(std::string& result, const StringConversionMagick& fmt, va_list list);
	extern std::string		FormatV		(const StringConversionMagick& fmt, va_list list);

	extern void				AppendFmt	(std::string& result, const char* fmt, ...)		__verify_fmt(2,3);
	extern std::string		Format		(const char* fmt, ...)							__verify_fmt(1,2);
	extern std::string  	trim		(const std::string& s, const char* delims = " \t\r\n");
	extern std::string  	toLower		(std::string s);
	extern std::string  	toUpper		(std::string s);
	extern std::string  	ReplaceCharSet(std::string srccopy, const char* to_replace, char new_ch);
	extern bool				globMatch	(char const* pattern, char const* candidate);

	extern bool getBoolean(const StringConversionMagick& left, bool* parse_error=nullptr);
	inline std::tuple<bool, bool> getBoolean(const StringConversionMagick& left, bool defbool) {
		bool error;
		auto result = getBoolean(left, &error);
		return { error ? defbool : result, error };
	}


	inline std::string	ReplaceString(std::string subject, const std::string& search, const std::string& replace) {
		size_t pos = 0;
		while ((pos = subject.find(search, pos)) != std::string::npos) {
			subject.replace(pos, search.length(), replace);
			pos += replace.length();
		}
		return subject;
	}

	inline std::string	ReplaceCase(std::string subject, const std::string& search, const std::string& replace) {
		const char* pos = subject.data();

		while ((pos = strcasestr(pos, search.c_str())) != 0) {
			subject.replace(pos-subject.c_str(), search.length(), replace);
			pos += search.length();
		}
		return subject;
	}
}

extern uint32_t cppStrToU32(const StringConversionMagick& src, char** endptr = nullptr);

extern int strcpy_ajek(char* dest, int destlen, const char* src);
template<int size> __nodebug
int strcpy_ajek(char (&dest)[size], const char* src)
{
	return strcpy_ajek(dest, size, src);
}

// returns TRUE if the argument is nullptr or empty string.
inline bool strIsEmpty(StringConversionMagick const& view) {
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
#	define cFmtStr(...)		            (VERIFY_PRINTF_ON_MSVC(__VA_ARGS__), StringUtil::Format(__VA_ARGS__).c_str())
#	define AppendFmtStr(dest, fmt, ...)	(VERIFY_PRINTF_ON_MSVC(fmt, __VA_ARGS__), StringUtil::AppendFmt(dest, fmt, ## __VA_ARGS__))
#else
#	define sFmtStr(...)		            (StringUtil::Format(__VA_ARGS__)        )
#	define cFmtStr(...)		            (StringUtil::Format(__VA_ARGS__).c_str())
#	define AppendFmtStr(dest, fmt, ...)	(StringUtil::AppendFmt(dest, fmt, ## __VA_ARGS__))
#endif

// Custom string to integer conversions: sj for intmax_t, uj for uintmax_t
// Also support implicit conversion from std::string
__nodebug inline intmax_t strtosj(const StringConversionMagick& src, char** meh, int radix) {
	return src.c_str() ? strtoll(src.c_str(), meh, radix) : 0;
}

__nodebug inline uintmax_t strtouj(const StringConversionMagick& src, char** meh, int radix) {
	return src.c_str() ? strtoul(src.c_str(), meh, radix) : 0;
}

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
