#include "StdStringArg.h"
#include "jfmt.h"
#include "ctype.h"

#include <type_traits>

// Custom string to integer conversion, simplified to return large types (defers clamping to caller) and added
// support fot C++ string_view, to allow parsing strings without tokenizing them using NUL.

// Historical Implementation Notes:
// Original intent was to just borrow BSD's strtol implementation and add support for string_view, but a cursory
// inspection of the implementation revealed some basic flaws in how it calculates overflow, particularly of
// negative unsigned values but also involving some undefined behavior around SIGNED MODULO (oops). The code in
// question dates to around 1992 and includes a long comment explaining why we should trust that it works (hint:
// comments explaining broken code shouldn't automatically make it more trustworthy). Apple even tried to fix it in
// somewhat hilarious fashion in their BSD port, by throwing like 6 random casts at the problem, creating a series
// of bizarre logics that I didn't bother trying to sort whether they actually solved the underlying problems or not.
// Our implementation just tests for signbit inversion which, afaik, works in all signed arithmetic schemes (ones and
// twos compliment) though I have no direct experience working with ones compliment to verify.
//
//   --jstine Dec 2022

template< typename T > __always_inline
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
ResultType _generic_strtoj(StringViewSpecificArg<allowCString, allowStringView> srcmagick, char** endptr, int base=0)
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
	for (;;) {
		int c = srcmagick.read_next();
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

	if (endptr) {
		*endptr = const_cast<char*>(any ? (srcmagick.data() + (srcmagick.readpos()-1)) : nullptr);
	}
	return (acc);
}

intmax_t strtosj(char const* src, char** endptr, int radix) {
	return _generic_strtoj<intmax_t, true, false>(src, endptr, radix);
}

uintmax_t strtouj(char const* src, char** endptr, int radix) {
	return _generic_strtoj<uintmax_t, true, false>(src, endptr, radix);
}

intmax_t strtosj(StringViewTempArg src, char** endptr, int radix) {
	return _generic_strtoj<intmax_t, false, true>(src.string_view(), endptr, radix);
}

uintmax_t strtouj(StringViewTempArg src, char** endptr, int radix) {
	return _generic_strtoj<uintmax_t, false, true>(src.string_view(), endptr, radix);
}


