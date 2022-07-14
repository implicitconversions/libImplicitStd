
#include "StringUtil.h"
#include "icy_assert.h"

#include <cstring>
#include <cstdarg>

#if !defined(PLATFORM_MSW)
#   if defined(_MSC_VER)
#       define PLATFORM_MSW     1
#   else
#       define PLATFORM_MSW     0
#   endif
#endif

#if defined(_MSC_VER)
#   if !defined(__always_inline)
#       define __always_inline				__forceinline
#   endif
#   if !defined(__noinline)
#	    define	__noinline					__declspec(noinline)
#   endif
#endif

#if !defined(elif)
#	define elif		else if
#endif

#if PLATFORM_MSW
#include <Shlwapi.h>

#pragma comment(lib, "Shlwapi")

char *_stristr(const char *haystack, const char *needle)
{
	// Windows has no libc equivalent of strcasestr(), but it does have this in the ShlwAPI...
	return ::StrStrIA(haystack, needle);
}
#endif

// Basically an extension to strtoul() which supports C++14 formatting extension for binary.
uint32_t cppStrToU32(const StringConversionMagick& srcmagick, char** endptr) {
	const char* src = srcmagick.c_str();

	if (strncmp(src, "0b", 2) == 0) {
		// binary notation support.
		src += 2;
		const char* startpos = src;
		uint32_t result = 0;
		while (src[0]) {
			if   (src[0] == '1' )   { result = (result << 1) | 1; }
			elif (src[0] == '0' )   { result = (result << 1) | 0; }
			elif (src[0] == '\'')   { /* ignored */ }
			else {
				// parse error, essentially.
				break;
			}
			++src;
		}
		if (endptr) {
			*endptr = (char*)((src == startpos) ? srcmagick.c_str() : src);
		}
		return result;
	}

	return strtoul(src, endptr, 0);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// Why strcpy_ajek?
//
// strcpy_s and strncpy_s behavior is often problematic if we aren't setting up a constraints handler, which we could
// do but that tends to open its own can of worms (it changes behavior of a number of _s functions).  It's insistence
// on making truncation difficult is not much help for security either.  It's just more work for us.
//
//   - Leaving a dest string empty because truncation would have occurred is as potentially "dangerous" as truncating
//     a string.  Moreover, the sort of 'risks' associated with truncation are super-obscure, easy to avoid in any number
//     of other more sensible ways and, most importantly, don't apply at all to game/emu development.
//
//   - Requiring the programmer to explicitly specify the length of  the dest buffer minus 1 re-introduces one of
//     the very problems that these functions were meant to solve: people forgetting to do -1 when specifying dest buffer
//     size to various APIs.  I've encountered so many instances of people typo'ing their strncpy_s usage such that
//     it still failed to truncate.  It gets especially ugly if you want to allow truncation while also concatenating
//     a couple of strings together.  Ugh.  --jstine
//

__nodebug int strcpy_ajek(char* dest, int destlen, const char* src)
{
	if (!dest || !src) return 0;
	if (!destlen) return 0;

	char* ret = dest;
	int pos = 0;
	while(pos < destlen)
	{
		dest[pos] = src[pos];
		if (!src[pos]) return pos;
		++pos;
	}
	//dbg_check(pos == destlen);

	// truncation scenario, ensure null terminator...
	dest[destlen-1] = 0;
	return destlen-1;
}

namespace StringUtil {

__nodebug std::string toLower(std::string src)
{
	// UTF8 friendly variety!  Can't use std::transform because tolower() needs to operate on
	// unsigned character codes.
	for (char& c : src) {
		c = ::tolower(uint8_t(c));
	}
	return src;
}

__nodebug std::string toUpper(std::string src)
{
	// UTF8 friendly variety!  Can't use std::transform because tolower() needs to operate on
	// unsigned character codes.
	for (char& c : src) {
		c = ::toupper(uint8_t(c));
	}
	return src;
}

__nodebug std::string trim(const std::string& s, const char* delims) {
	int sp = 0;
	int ep = int(s.length()) - 1;
	for (; ep >= 0; --ep)
		if (!strchr(delims, (uint8_t)s[ep]))
			break;

	for (; sp <= ep; ++sp)
		if (!strchr(delims, (uint8_t)s[sp]))
			break;

	return s.substr(sp, ep - sp + 1);
}

void AppendFmtV(std::string& result, const StringConversionMagick& fmt, va_list list)
{
	if (fmt.empty()) return;

	int destSize = 0;
	va_list argcopy;
	va_copy(argcopy, list);
	destSize = vsnprintf(nullptr, 0, fmt.c_str(), argcopy);
	va_end(argcopy);

	dbg_check(destSize >= 0, "Invalid string formatting parameters");

	// vsnprintf doesn't count terminating '\0', and resize() doesn't expect it either.
	// Thus, the following resize() will ensure +1 room for the null that vsprintf_s
	// will write.

	auto curlen = result.length();
	result.resize(destSize+curlen);
	vsprintf_s(const_cast<char*>(result.data() + curlen), destSize+1, fmt.c_str(), list );
}

__nodebug void AppendFmt(std::string& result, const char* fmt, ...)
{
	va_list list;
	va_start(list, fmt);
	StringUtil::AppendFmtV(result, fmt, list);
	va_end(list);
}


__nodebug std::string FormatV(const StringConversionMagick& fmt, va_list list)
{
	std::string result;
	AppendFmtV(result, fmt, list);
	return result;
}

__nodebug std::string Format(const char* fmt, ...)
{
	va_list list;
	va_start(list, fmt);
	auto result = StringUtil::FormatV(fmt, list);
	va_end(list);
	return result;
}

bool getBoolean(const StringConversionMagick& left, bool* parse_error)
{
	const char* woo = left.c_str();

	if (parse_error) {
		*parse_error = 0;
	}

	if (woo[0] && !woo[1]) {
		if (woo[0] == '0') return false;
		if (woo[0] == '1') return true;
		if (parse_error) {
			*parse_error = 1;
		}
		return false;
	}
	if (strcasecmp(woo, "true")		== 0) return true;
	if (strcasecmp(woo, "on")		== 0) return true;
	if (strcasecmp(woo, "false")	== 0) return false;
	if (strcasecmp(woo, "off")		== 0) return false;

	if (parse_error) {
		*parse_error = 1;
	}
	return false;
}

std::string ReplaceCharSet(std::string srccopy, const char* to_replace, char new_ch) {
	for (char& ch : srccopy) {
		ch = tolower(uint8_t(ch));
		if (strchr(to_replace, uint8_t(ch))) {
			ch = '-';
		}
	}
	return srccopy;
}

} // namespace StringUtil


__nodebug bool u8_nul_or_whitespace(uint8_t ch) {
	return (ch == 0) || isspace(ch);
}

// Supports h m s ms  (todo: add support for D M Y for days/months/years?)
// Expects the endptr as returned from strtod or strtoj.
// returns a scalar normalized from seconds, such that 's' = 1.0 and 'ms' = 0.0001 and 'h' = 60*60, etc.
// returns 0.0 if the postfix is null or whitespace.
// returns -1.0 if the postfix is invalid.
double CvtTimePostfixToScalar(char const* endptr) {
	if (!endptr || isspace(*endptr)) {
		return 1.0;
	}
	if (endptr[0] == 's') {
		if (endptr[1]) {
			return -1.0;
		}
		return 1.0;
	}
	if (endptr[0] == 'h') {
		if (endptr[1]) {
			return -1.0;
		}
		return 1.0 * 60 * 60;
	}
	if (endptr[0] == 'm') {
		if (!endptr[1]) {
			return 1.0 * 60;		// minute
		}
		if (endptr[1] == 's') {
			if (endptr[2]) {
				return -1.0;
			}
			return 1.0 / 1000;		// millisecond
		}
	}

	return -1.0;
};

// Supports mib/kib/gib and mb/gb/kb
// Expects the endptr as returned from strtod or strtoj.
// returns a scalar normalized from bytes
// returns 1.0 if the postfix is null or whitespace.
// returns -1.0 if the postfix is invalid.
double CvtNumericalPostfixToScalar(char const* endptr) {
	if (!endptr || isspace((uint8_t)*endptr)) {
		return 1.0;
	}

	if (strnlen(endptr,4) > 3) {
		return -1;
	}

	auto digits = std::string_view{endptr}.substr(0, 3);

	if (digits == "g")   { return 1024*1024*1024; }
	if (digits == "gib") { return 1024*1024*1024; }
	if (digits == "gb" ) { return 1000*1000*1000; }

	if (digits == "m")   { return 1024*1024; }
	if (digits == "mib") { return 1024*1024; }
	if (digits == "mb" ) { return 1000*1000; }

	if (digits == "k")   { return 1024; }
	if (digits == "kib") { return 1024; }
	if (digits == "kb" ) { return 1000; }

	return -1.0;
};

std::tuple<intmax_t, bool> StrParseSizeArg(char const* src) {

	if (!src || !src[0]) {
		return {};
	}

	char* endptr = nullptr;
	if (auto newval = strtosj(src, &endptr, 0); endptr && endptr != src) {
		newval *= CvtNumericalPostfixToScalar(endptr);
		if (newval > 0) {
			return { newval, true };
		}
	}

	return {};
}
