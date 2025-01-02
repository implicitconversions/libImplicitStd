// Copyright (c) 2021-2025, Implicit Conversions, Inc. Subject to the MIT License. See LICENSE file.

#include "StringUtil.h"
#include "icy_assert.h"

#include <cstring>
#include <cstdarg>
#include <charconv>
#include <iomanip>
#include <limits>
#include <optional>

#include "StdString.hxx"

#if !defined(PLATFORM_MSW)
#   if defined(_MSC_VER)
#       define PLATFORM_MSW     1
#   else
#       define PLATFORM_MSW     0
#   endif
#endif

#if defined(_MSC_VER)
#   if !defined(yesinline)
#       define yesinline				__forceinline
#   endif
#   if !defined(__noinline)
#	    define	__noinline					__declspec(noinline)
#   endif
#endif

#if !defined(elif)
#	define elif		else if
#endif

#if PLATFORM_MSW
#ifndef NOMINMAX
	#define NOMINMAX 1
#endif
#include <Shlwapi.h>

#pragma comment(lib, "Shlwapi")

char *_stristr(const char *haystack, const char *needle)
{
	// Windows has no libc equivalent of strcasestr(), but it does have this in the ShlwAPI...
	return ::StrStrIA(haystack, needle);
}
#endif

// for easy return of std::string as a const reference. Don't reference this from code that might
// execute pre-main.
std::string g_empty_stdstring;

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
//     size to various APIs. It gets especially ugly if you want to allow truncation while also concatenating
//     a couple of strings together.
//

int strcpy_ajek(char* dest, int destlen, const char* src)
{
	if (!dest || !src) return 0;
	if (!destlen) return 0;

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

std::string toLower(std::string src)
{
	// UTF8 friendly variety!  Can't use std::transform because tolower() needs to operate on
	// unsigned character codes.
	for (char& c : src) {
		c = ::tolower(uint8_t(c));
	}
	return src;
}

std::string toUpper(std::string src)
{
	// UTF8 friendly variety!  Can't use std::transform because tolower() needs to operate on
	// unsigned character codes.
	for (char& c : src) {
		c = ::toupper(uint8_t(c));
	}
	return src;
}

std::string trim(const std::string& s, const char* delims) {
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

template void AppendFmtV<std::string>(std::string& result, const StringConversionMagick& fmt, va_list list);
template void AppendFmt <std::string>(std::string& result, const char* fmt, ...);


yesinline
std::string FormatV(const StringConversionMagick& fmt, va_list list)
{
	std::string result;
	AppendFmtV(result, fmt, list);
	return result;
}

__va_inline
std::string Format(const char* fmt, ...)
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

ptrdiff_t CompareCase(std::string_view lval, std::string_view rval)
{
	uint8_t const* lvp = (uint8_t*)lval.data();
	uint8_t const* rvp = (uint8_t*)rval.data();

	auto maxlen = std::min(lval.size(), rval.size());

	for (; maxlen != 0; maxlen--, lvp++, rvp++) {
		auto lvl = tolower(*lvp);
		auto rvl = tolower(*rvp);
		if (auto diff = lvl - rvl; diff) {
			return diff;
		}
	}
	return 0;
}

ptrdiff_t FindFirstCase(std::string_view s, std::string_view find) {
	uint8_t const* fptr = (uint8_t*)find.data();
	uint8_t const* sptr = (uint8_t*)s.data();

	int first_c = tolower(*fptr);
	auto remain = (ptrdiff_t)(s.size() - find.size());

	while (remain >= 0) {
		if (tolower(*sptr) == first_c) {
			if (strncasecmp(sptr, fptr, find.size()) == 0) {
				return sptr - (uint8_t*)s.data();
			}
		}
		--remain;
		++sptr;
	}

	return -1;
}

} // namespace StringUtil

yesinline
bool u8_nul_or_whitespace(uint8_t ch) {
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

// performs globbing on pattern strings, including support for escaping (backslash). This means that the following
// characters can be matched if preceeded by backslash:: * ? [ ]
//    Example:  "Question\?"
//
// This function is suitable for glob-matching plain text C/C++ identifiers and most filenames.
//
bool StringUtil::globMatch(char const* pattern, char const* candidate) {
	auto* globwalk = pattern;
	auto* namewalk = candidate;

	if (!globwalk || !namewalk) {
		return false;			// nullptr does not match nullptr.
	}

	if (!globwalk[0]) {
		return !namewalk[0];	// empty string DOES match empty string.
	}

	if (!namewalk[0]) {
		return false;
	}

	while (true) {
		// TODO: implement bracket matching, which is useful for case sensitivity, eg [Aa]
		//if (expect_false(*globwalk == '[')) {
		//	++globwalk;
		//	char matches[256];
		//	int mid = 0;
		//	while(*globwalk && *globwalk != ']' && (mid < std::ssize(matches)-1)) {
		//		matches[mid++] = *globwalk;
		//	}
		//}

		if (expect_false(*globwalk == '\\')) {
			if (strchr("*?[]", globwalk[1])) {
				++globwalk;
			}
			if (*globwalk != *namewalk) {
				return false;
			}
		}
		elif (expect_false(*globwalk == '*')) {
			char match_next = globwalk[1];

			while(namewalk[1] && namewalk[1] != match_next) {
				++namewalk;
			}
		}
		elif (*globwalk != *namewalk && *globwalk != '?') {
			return false;
		}

		++namewalk;
		++globwalk;

		if (*namewalk == 0 && *globwalk == 0) {
			return true;
		}
	}
}

namespace {

	constexpr uint8_t kUtf8Safe2ByteSequence[2] = { 0xC2, 0xBF };
	constexpr uint8_t kUtf8Safe3ByteSequence[3] = { 0xE0, 0xA0, 0x86 };
	constexpr uint8_t kUtf8Safe4ByteSequence[4] = { 0xF0, 0x90, 0x8C, 0xB8 };

	std::string Ascii8ToUtf8(char ch) {
		std::string result;
		int iChar = ch;
		if (iChar < 0) {
			iChar += 256;
		}
		if (iChar < 128) {
			result.append(&ch, 1);
		}
		else {
			const char utf8Char[2] = {
				char(0b11000000 | (iChar >> 6)), // The 2 upper bits mark it as a 2 byte character, insert the upper 2 bits of the ascii8 character
				char(0b10000000 | int(iChar & 0b00111111)), // The bottom 6 bits of the ascii8 character
			};

			result.append(utf8Char, std::ssize(utf8Char));
		}

		return result;
	}

	bool IsUtf8CharacterStartByte(uint8_t ch, int& sequenceLength) {
		// 0xxxxxxx - 1-byte sequence - traditional ASCII
		if ((ch & 0b1000'0000) == 0b0000'0000) {
			sequenceLength = 1;
		}
		// 110xxxxx - 2-byte sequence
		else if ((ch & 0b1110'0000) == 0b1100'0000) {
			sequenceLength = 2;
		}
		// 1110xxxx - 3-byte sequence
		else if ((ch & 0b1111'0000) == 0b1110'0000) {
			sequenceLength = 3;
		}
		// 11110xxx - 4-byte sequence
		else if ((ch & 0b1111'1000) == 0b1111'0000) {
			sequenceLength = 4;
		}
		// Invalid
		else {
			sequenceLength = 1;
			return false;
		}

		return true;
	}

	bool IsValidUtf8CharacterSequence(const uint8_t* ch, int& supposedSequenceLength) {
		if (!IsUtf8CharacterStartByte(*ch, supposedSequenceLength)) {
			return false;
		}

		// Skip the start byte
		ch++;
		int sequenceRemaining = supposedSequenceLength - 1;

		while (sequenceRemaining > 0 && *ch != '\0') {
			// Make sure each byte is of the form 10xxxxxx
			if ((*ch & 0b1100'0000) != 0b1000'0000) {
				return false;
			}
			ch++;
			sequenceRemaining--;
		}

		// End of string reached but byte sequence not complete
		if (sequenceRemaining > 0) {
			return false;
		}

		return true;
	}
}

std::string SanitizeUtf8(const std::string& str) {

	std::string result;

	const uint8_t* cur = reinterpret_cast<const uint8_t*>(str.data());

	while (*cur != '\0') {

		int sequenceLength = 0;

		// Valid sequence, copy into the result
		if (IsValidUtf8CharacterSequence(cur, sequenceLength)) {
			result.append(reinterpret_cast<const char*>(cur), sequenceLength);
		}
		// Invalid single character sequence - assume we got ASCII8
		else if (sequenceLength == 1) {
			result.append(Ascii8ToUtf8(*cur));
		}
		// Insert a safe character
		else {
			switch (sequenceLength) {
				case 2: {
					result.append(reinterpret_cast<const char*>(kUtf8Safe2ByteSequence), std::ssize(kUtf8Safe2ByteSequence));
					break;
				}
				case 3: {
					result.append(reinterpret_cast<const char*>(kUtf8Safe3ByteSequence), std::ssize(kUtf8Safe3ByteSequence));
					break;
				}
				case 4: {
					result.append(reinterpret_cast<const char*>(kUtf8Safe4ByteSequence), std::ssize(kUtf8Safe4ByteSequence));
					break;
				}
				default: {
					// This should never happen
					break;
				}
			}
		}

		// Move to the next character start
		for (int i = 0; i < sequenceLength; ++i) {
			if (*cur == '\0') {
				break;
			}
			cur++;
		}
	}

	return result;
}

std::optional<bool> StringUtil::_template_impl::ConvertToBool(StringConversionMagick const& rval) {
	bool result = 1;
	if (!rval.empty()) {
		bool parse_error;
		result = StringUtil::getBoolean(rval, &parse_error);
		if (parse_error) {
			errno = EINVAL;
			return {};
		}
	}
	return { result };
}

std::optional<double> StringUtil::_template_impl::ConvertFromString_f64(StringConversionMagick const& rval) {
	char *endptr = nullptr;
	const char* valstr = rval.c_str();
	auto value = strtod(valstr, &endptr);
	if (endptr == valstr) {
		errno = EINVAL;
		return {};
	}
	return { value };
}

std::optional<float> StringUtil::_template_impl::ConvertFromString_f32(StringConversionMagick const& rval) {
	char *endptr = nullptr;
	const char* valstr = rval.c_str();
	auto value = strtof(valstr, &endptr);
	if (endptr == valstr) {
		errno = EINVAL;
		return {};
	}
	return { value };
}

template<> std::optional<bool    > StringUtil::Parse(StringConversionMagick const& rval) { return _template_impl::ConvertToBool(rval);	}
template<> std::optional<uint32_t> StringUtil::Parse(StringConversionMagick const& rval) { return _template_impl::ConvertFromString_integrals<uint32_t>(rval); }
template<> std::optional< int32_t> StringUtil::Parse(StringConversionMagick const& rval) { return _template_impl::ConvertFromString_integrals< int32_t>(rval); }
template<> std::optional<uint64_t> StringUtil::Parse(StringConversionMagick const& rval) { return _template_impl::ConvertFromString_integrals<uint64_t>(rval); }
template<> std::optional< int64_t> StringUtil::Parse(StringConversionMagick const& rval) { return _template_impl::ConvertFromString_integrals< int64_t>(rval); }
template<> std::optional<uint16_t> StringUtil::Parse(StringConversionMagick const& rval) { return _template_impl::ConvertFromString_integrals<uint16_t>(rval); }
template<> std::optional< int16_t> StringUtil::Parse(StringConversionMagick const& rval) { return _template_impl::ConvertFromString_integrals< int16_t>(rval); }
template<> std::optional<uint8_t > StringUtil::Parse(StringConversionMagick const& rval) { return _template_impl::ConvertFromString_integrals<uint8_t >(rval); }
template<> std::optional< int8_t > StringUtil::Parse(StringConversionMagick const& rval) { return _template_impl::ConvertFromString_integrals< int8_t >(rval); }
template<> std::optional<float>    StringUtil::Parse(StringConversionMagick const& rval) { return _template_impl::ConvertFromString_f32(rval); }
template<> std::optional<double>   StringUtil::Parse(StringConversionMagick const& rval) { return _template_impl::ConvertFromString_f64(rval); }

std::string StringUtil::LineNumberString(const char* str) {
	std::stringstream input(str);
	std::stringstream output;
	std::string line;

	int lineNumber = 1;
	while (std::getline(input, line)) {
		output << std::setw(4) << lineNumber++ << ":  " << line << std::endl;
	}

	return output.str();
}
