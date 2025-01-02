// Copyright (c) 2021-2025, Implicit Conversions, Inc. Subject to the MIT License. See LICENSE file.

#include "StdStringArg.h"
#include "jfmt.h"
#include "ctype.h"

#include <limits>
#include <type_traits>

// Custom string to integer conversion, simplified to return large types (defers clamping to caller) and added
// support fot C++ string_view, to allow parsing strings without tokenizing them using NUL.

template< typename T > yesinline
std::make_unsigned_t<T> to_unsigned(T src) {
	return (std::make_unsigned_t<T>)src;
}

/*
 * Convert a string to a long long integer.
 *
 * Ignores `locale' stuff.  Assumes that the upper and lower case
 * alphabets and digits are each contiguous.
 */

template<typename ResultType, bool allowCString=true, bool allowStringView=true>
ResultType _generic_strtoj(StringViewSpecificArg<allowCString, allowStringView> srcmagick, int* charsConsumed, int base=0)
{
	constexpr bool isSigned = std::is_signed_v<ResultType>;
	int neg = 1;

	/*
	 * Skip white space and pick up leading +/- sign if any.
	 * If base is 0, allow 0x for hex and 0 for octal, else
	 * assume decimal; if base is already 16, allow 0x.
	 */

	int c;
	while (isspace(c=srcmagick.read_next()));

	if (c == '-') {
		neg = -1;
		c = srcmagick.read_next();
	} else if (c == '+')
		c = srcmagick.read_next();

	if (c == '0') {
		auto base_code = srcmagick.peek_next();

		if ((base == 0 || base == 16) &&
			(base_code == 'x' || base_code == 'X')) {
			srcmagick.read_next();
			base = 16;
		}

		// C++ extension: support binary! eg 0b1111
		if ((base == 0 || base == 2) &&
			c == '0' && (base_code == 'b' || base_code == 'B')) {
			srcmagick.read_next();
			base = 2;
		}

		if (base == 0)
			base = 8;
	}

	if (base == 0)
		base = 10;

	// accumulate values into unsigned type regardless of sign.
	// Sign is applied at the end, so if reading a signed value it can be stored in an unsigned
	// type and this makes checking for overflow easier.

	ResultType acc  = 0;
	ResultType oacc = 0;
	int any = 0;
	bool erange = false;
	for (;;c = srcmagick.read_next()) {
		if (!c) {
			break;
		}

		if (isdigit(c)) {
			c -= '0';
		}
		else if (isalpha(c)) {
			c -= isupper(c)
				? ('A' - 10)
				: ('a' - 10);
		}
		// C++ extension: ignore single quote, it's part of the number.
		else if (c == '\'') {
			continue;
		}
		else {
			break;
		}

		if (c >= base)
			break;


		intmax_t addval = c * neg;
		acc *= base;
		acc += addval;

		any = 1;

		// checking for overflow is only valid if oacc is non-zero.
		// Note that it's possible to hit oacc=0 condition again later, but only after erange has already
		// been flagged true, at which point accurate checking of overflow is moot.
		if (expect_true(oacc)) {
			constexpr auto bits_in_type = sizeof(ResultType) * 8;
			if constexpr (isSigned) {
				// if the sign bit changes, it's an overflow.
				if ((to_unsigned(acc ^ oacc) >> (bits_in_type-1)) == 1) {
					erange = true;
				}
			}
			else {
				auto signdiff = acc ^ oacc;
				if (signdiff ^ addval)
				if ((acc * neg) < (oacc * neg)) {
					erange = true;
				}
			}
		}
		oacc = acc;
	}

	if (erange) {
		if (isSigned) {
			acc = (neg < 0) ? std::numeric_limits<ResultType>().min() : std::numeric_limits<ResultType>().max();
		}
		else {
			acc = std::numeric_limits<ResultType>().max();
		}
		errno = ERANGE;
	}

	if (charsConsumed) {
		*charsConsumed = any ? (srcmagick.readpos()-1) : 0;
	}
	return acc;
}

intmax_t strtosj(char const* src, char** endptr, int radix) {
	int consumed = 0;
	auto result = _generic_strtoj<intmax_t, true, false>(src, &consumed, radix);
	if (endptr) {
		*endptr = const_cast<char*>(src) + consumed;
	}
	return result;
}

uintmax_t strtouj(char const* src, char** endptr, int radix) {
	int consumed = 0;
	auto result = _generic_strtoj<uintmax_t, true, false>(src, &consumed, radix);
	if (endptr) {
		*endptr = const_cast<char*>(src) + consumed;
	}
	return result;
}

intmax_t strtosj(std::string_view src, int* charsConsumed, int radix) {
	return _generic_strtoj<intmax_t, false, true>(src, charsConsumed, radix);
}

uintmax_t strtouj(std::string_view src, int* charsConsumed, int radix) {
	return _generic_strtoj<uintmax_t, false, true>(src, charsConsumed, radix);
}

intmax_t  strtosj(char const* src,       int radix) { return strtosj(src, nullptr, radix); }
intmax_t  strtosj(std::string_view src,  int radix) { return strtosj(src, nullptr, radix); }
uintmax_t strtouj(char const* src,       int radix) { return strtouj(src, nullptr, radix); }
uintmax_t strtouj(std::string_view src,  int radix) { return strtouj(src, nullptr, radix); }

